// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "transpiler/netlist_utils.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "transpiler/graph.h"
#include "xls/common/status/status_macros.h"
#include "xls/netlist/cell_library.h"
#include "xls/netlist/function_extractor.h"
#include "xls/netlist/lib_parser.h"
#include "xls/netlist/netlist.h"
#include "xls/netlist/netlist_parser.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

using ::xls::netlist::AbstractCellLibrary;
using ::xls::netlist::CellLibraryProto;
using ::xls::netlist::cell_lib::CharStream;
using ::xls::netlist::rtl::AbstractCell;
using ::xls::netlist::rtl::AbstractNetlist;
using ::xls::netlist::rtl::AbstractNetRef;
using ::xls::netlist::rtl::AbstractParser;
using ::xls::netlist::rtl::NetDeclKind;
using ::xls::netlist::rtl::Scanner;

absl::StatusOr<AbstractCellLibrary<bool>> ParseCellLibrary(
    const absl::string_view cell_library_text) {
  XLS_ASSIGN_OR_RETURN(CharStream char_stream,
                       CharStream::FromText(std::string(cell_library_text)));
  CellLibraryProto cell_library_proto =
      *xls::netlist::function::ExtractFunctions(&char_stream);

  XLS_ASSIGN_OR_RETURN(
      AbstractCellLibrary<bool> cell_library,
      AbstractCellLibrary<bool>::FromProto(cell_library_proto, false, true));

  return cell_library;
}

absl::StatusOr<std::unique_ptr<AbstractNetlist<bool>>> ParseNetlist(
    xls::netlist::AbstractCellLibrary<bool>& cell_library,
    const absl::string_view netlist_text) {
  Scanner scanner(netlist_text);
  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<AbstractNetlist<bool>> parsed_netlist_ptr,
      AbstractParser<bool>::ParseNetlist(&cell_library, &scanner, false, true));
  if (parsed_netlist_ptr->modules().length() > 1) {
    return absl::InvalidArgumentError(
        "Multiple module definitions not supported");
  }

  return parsed_netlist_ptr;
}

absl::StatusOr<Graph<absl::string_view, int>> ParseNetlistToGraph(
    const xls::netlist::rtl::AbstractModule<bool>& module) {
  // The netlist graph is bipartite between cells and intermediate
  // wires/inputs/outputs. A wire connects the output of one cell to inputs of
  // other cells. we need to keep track of which cell is connected
  // to a given output wire, so that when we process a cell that uses that wire
  // as an input, we know which cell to use as the other end of the edge.
  absl::flat_hash_map<absl::string_view, absl::string_view>
      output_wire_to_cell_name;
  for (const auto& cell : module.cells()) {
    for (const auto& pin : cell->outputs()) {
      if (pin.netref->kind() == NetDeclKind::kWire ||
          pin.netref->kind() == NetDeclKind::kOutput) {
        output_wire_to_cell_name[pin.netref->name()] = cell->name();
      }
    }
  }

  Graph<absl::string_view, int> graph;
  for (const auto& cell : module.cells()) {
    bool uses_wire = false;
    for (const auto& pin : cell->inputs()) {
      // The netlist parser does not distinguish between constant inputs and
      // wire inputs (constants have type kWire), so we use the name as a proxy.
      if ((pin.netref->kind() == NetDeclKind::kWire ||
           pin.netref->kind() == NetDeclKind::kOutput) &&
          !absl::StrContains(pin.netref->name(), "constant")) {
        // An edge is added from the cell that produces output at the output_pin
        // to the current cell.
        AbstractNetRef<bool> netref_to_lookup = pin.netref;
        while (!output_wire_to_cell_name.contains(netref_to_lookup->name())) {
          // try looking backwards through reassignments of wires
          if (!module.assigns().contains(netref_to_lookup)) {
            return absl::InvalidArgumentError(absl::StrFormat(
                "usage of uninitialized wire %s", netref_to_lookup->name()));
          }
          netref_to_lookup = module.assigns().at(netref_to_lookup);
        }

        auto source = output_wire_to_cell_name.at(netref_to_lookup->name());
        graph.AddVertex(source, 1);
        graph.AddVertex(cell->name(), 1);
        graph.AddEdge(source, cell->name());
        uses_wire = true;
      }
    }

    if (!uses_wire) {
      // Some cells may have only in/out/constant arguments, and these can be
      // processed in any order. There is no wire dependency with another cell.
      graph.AddVertex(cell->name(), 1);
    }
  }
  return graph;
}

absl::StatusOr<std::vector<std::string>> TopoSortedCellNames(
    const xls::netlist::rtl::AbstractModule<bool>& module) {
  XLS_ASSIGN_OR_RETURN(auto graph, ParseNetlistToGraph(module));
  XLS_ASSIGN_OR_RETURN(auto topo_order, graph.TopologicalSort());
  std::vector<std::string> output;
  output.reserve(topo_order.size());
  for (const auto& cell_name : topo_order) {
    output.push_back(std::string(cell_name));
  }
  return output;
}

// Returns a vector of vectors of node names, where each vector represents
// a different level in the graph. Nodes in the same level can be executed in
// parallel.
absl::StatusOr<std::vector<std::vector<std::string>>> LevelSortedCellNames(
    const xls::netlist::rtl::AbstractModule<bool>& module) {
  XLS_ASSIGN_OR_RETURN(auto graph, ParseNetlistToGraph(module));
  XLS_ASSIGN_OR_RETURN(auto level_sorted_nodes, graph.SortGraphByLevels());
  std::vector<std::vector<std::string>> output(level_sorted_nodes.size());
  for (int level = 0; level < level_sorted_nodes.size(); ++level) {
    for (const auto& cell_name : level_sorted_nodes[level]) {
      output[level].push_back(std::string(cell_name));
    }
  }
  return output;
}

absl::StatusOr<int> NetRefIdToNumericId(absl::string_view netref_id) {
  absl::string_view stripped =
      absl::StripSuffix(absl::StripPrefix(netref_id, "_"), "_");
  int output = 0;
  if (!absl::SimpleAtoi(stripped, &output)) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Netlist contains non-numeric netref id. Expected an expression "
        "like '_0123_', but got '%s'",
        netref_id));
  }
  return output;
}

absl::StatusOr<int> NetRefIdToIndex(absl::string_view netref) {
  std::vector<std::string> tokens =
      absl::StrSplit(netref, absl::ByAnyChar("[]"));
  if (tokens.size() <= 1) {
    return 0;
  }
  int output;
  if (!absl::SimpleAtoi(tokens[1], &output)) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Non integral index value for netref $0; extracted: $1", netref,
        tokens[1]));
  }
  return output;
}

std::string NetRefStem(std::string_view netref) {
  size_t index = netref.find_first_of('[');
  if (index == std::string::npos) {
    // The `[` is not found, so assume the entire string refers to a single-bit
    // variable.
    return std::string(netref);
  }
  return std::string(netref.substr(0, index));
}

absl::StatusOr<int> ConstantToValue(std::string_view constant) {
  size_t start_index = constant.find_first_of('_');
  size_t end_index = constant.find_first_of('>');
  if (start_index == std::string::npos || end_index == std::string::npos ||
      end_index < start_index) {
    return absl::InvalidArgumentError(
        absl::StrFormat("Invalid constant. Expected an expression "
                        "like '<constant_1>', but got '%s'",
                        constant));
  }
  std::string value_as_str = std::string(
      constant.substr(start_index + 1, end_index - start_index - 1));
  int output;
  if (!absl::SimpleAtoi(value_as_str, &output)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("Constant expression contains non-numeric value '%s'. "
                        "in expression '%s'",
                        value_as_str, constant));
  }
  return output;
}

absl::StatusOr<std::string> ResolveNetRefName(
    const AbstractNetRef<bool>& netref, const CodegenTemplates& templates) {
  if (netref->kind() == NetDeclKind::kWire) {
    XLS_ASSIGN_OR_RETURN(int numeric_ref, NetRefIdToNumericId(netref->name()));
    return absl::Substitute(
        templates.PriorGateOutputReference(std::to_string(numeric_ref)));
  } else {
    return templates.InputOrOutputReference(netref->name());
  }
}

absl::StatusOr<std::vector<int>> ExtractPriorGateOutputIds(
    const AbstractCell<bool>* cell) {
  std::vector<int> gate_inputs;
  for (const AbstractCell<bool>::Pin& input : cell->inputs()) {
    if (absl::StartsWith(input.name, "P") ||
        absl::StrContains(input.netref->name(), "constant")) {
      continue;
      // Input and output references are not included, and we rule them out by
      // the wire type
    } else if (input.netref->kind() == NetDeclKind::kWire) {
      XLS_ASSIGN_OR_RETURN(int numeric_ref,
                           NetRefIdToNumericId(input.netref->name()));
      gate_inputs.push_back(numeric_ref);
    }
  }

  return gate_inputs;
}

absl::StatusOr<GateInputs> ExtractGateInputs(
    const AbstractCell<bool>* cell, const CodegenTemplates& templates) {
  // For a LUT cell, the table is a constant numeric value determined by the
  // trailing 2 (or 4, or 8, etc.) inputs, corresponding to the pins P0, P1,
  // etc., as defined in transpiler/yosys/lut_cells.liberty
  //
  // The LUT pins are in order from LSB to MSB (P0 is the LSB). They are
  // defined as separate pins because the value assigned to a pin must be a
  // single bit. In this case we need to combine all the constant single bits
  // into an integer, and pass that along to the backend API.
  uint64_t lut_definition = 0;
  int bit_posn = 0;
  std::vector<std::string> gate_inputs;
  for (const AbstractCell<bool>::Pin& input : cell->inputs()) {
    // "P" is the magic string, defined by the cell liberty file, that
    // contains the bits of the truth table.
    if (absl::StartsWith(input.name, "P")) {
      XLS_ASSIGN_OR_RETURN(int lut_bit, ConstantToValue(input.netref->name()));
      lut_definition |= (uint64_t)lut_bit << bit_posn;
      ++bit_posn;
    } else if (absl::StrContains(input.netref->name(), "constant")) {
      XLS_ASSIGN_OR_RETURN(int constant_input,
                           ConstantToValue(input.netref->name()));
      gate_inputs.push_back(templates.ConstantCiphertext(constant_input));
    } else {
      XLS_ASSIGN_OR_RETURN(std::string input_name,
                           ResolveNetRefName(input.netref, templates));
      gate_inputs.push_back(input_name);
    }
  }

  return GateInputs{.inputs = gate_inputs, .lut_definition = lut_definition};
}

// Extract the output data for a single cell
absl::StatusOr<GateOutput> ExtractGateOutput(const AbstractCell<bool>* cell) {
  auto& gate_output = cell->outputs().front().netref;
  bool is_output = gate_output->kind() == NetDeclKind::kOutput;
  bool is_single_bit = !absl::StrContains(gate_output->name(), '[');

  XLS_ASSIGN_OR_RETURN(int index,
                       is_output ? NetRefIdToIndex(gate_output->name())
                                 : NetRefIdToNumericId(gate_output->name()));
  return GateOutput{.name = gate_output->name(),
                    .is_single_bit = is_single_bit,
                    .index = index,
                    .is_output = is_output};
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
