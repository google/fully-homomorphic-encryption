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
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "xls/common/status/status_macros.h"
#include "xls/netlist/netlist.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

using ::xls::netlist::rtl::AbstractCell;
using ::xls::netlist::rtl::AbstractNetRef;
using ::xls::netlist::rtl::NetDeclKind;

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

absl::StatusOr<GateInputs> ExtractGateInputs(
    const AbstractCell<bool>* cell, const CodegenTemplates& templates) {
  // For a LUT cell, the table is a constant numeric value determined by the
  // trailing 2 (or 4, or 8, etc.) inputs, corresponding to the pins P0, P1,
  // etc., as defined in transpiler/yosys/lut_cells.liberty
  //
  // The LUT pins are in order from LSB to MSB (P0 is the LSB). They are
  // defined as separate pins because the value assigned to a pin must be a
  // single bit. In this case we need to combine all the constant single bits
  // into an integer, and pass that along to the jaxite API.
  uint64_t lut_definition = 0;
  int bit_posn = 0;
  std::vector<std::string> gate_inputs;
  for (const AbstractCell<bool>::Pin& input : cell->inputs()) {
    // "P" is the magic string, defined by the cell liberty file, that contains
    // the bits of the truth table.
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
