#include "transpiler/jaxite/yosys_transpiler.h"

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "transpiler/netlist_utils.h"
#include "transpiler/util/string.h"
#include "xls/common/status/status_macros.h"
#include "xls/netlist/cell_library.h"
#include "xls/netlist/function_extractor.h"
#include "xls/netlist/lib_parser.h"
#include "xls/netlist/netlist.h"
#include "xls/netlist/netlist_parser.h"

namespace fhe {
namespace jaxite {
namespace transpiler {

namespace {

using ::xls::netlist::AbstractCellLibrary;
using ::xls::netlist::CellLibraryProto;
using ::xls::netlist::cell_lib::CharStream;
using ::xls::netlist::rtl::AbstractCell;
using ::xls::netlist::rtl::AbstractModule;
using ::xls::netlist::rtl::AbstractNetlist;
using ::xls::netlist::rtl::AbstractNetRef;
using ::xls::netlist::rtl::AbstractParser;
using ::xls::netlist::rtl::NetDeclKind;
using ::xls::netlist::rtl::Scanner;

using ::fully_homomorphic_encryption::ToSnakeCase;
using ::fully_homomorphic_encryption::transpiler::CodegenTemplates;
using ::fully_homomorphic_encryption::transpiler::ConstantToValue;
using ::fully_homomorphic_encryption::transpiler::ExtractGateInputs;
using ::fully_homomorphic_encryption::transpiler::GateInputs;
using ::fully_homomorphic_encryption::transpiler::LevelSortedCellNames;
using ::fully_homomorphic_encryption::transpiler::NetRefIdToNumericId;
using ::fully_homomorphic_encryption::transpiler::NetRefStem;
using ::fully_homomorphic_encryption::transpiler::ResolveNetRefName;
using ::fully_homomorphic_encryption::transpiler::TopoSortedCellNames;

// `buffer` gates are no-ops, and can be replaced with a simple assignment in
// the generated Python code. The `buffer` name is fixed in the cell library,
// which is effectively an internal implementation detail for the user of the
// transpiler API. We hard code the name here for simplicity (though this would
// break if the name `buffer` changed in tfhe_cells.liberty). If no
// `buffer`-like cell is present in the cell library, Yosys will use assignment
// statements. We support both possibilities for completeness.
constexpr absl::string_view kBufferGateName = "buffer";

static auto kCellNameToJaxiteOp =
    absl::flat_hash_map<absl::string_view, absl::string_view>({
        {"and2", "and_"},
        {"andny2", "andny_"},
        {"andyn2", "andyn_"},
        {"inv", "not_"},
        {"imux2", "cmux_"},
        {"nand2", "nand_"},
        {"nor2", "nor_"},
        {"or2", "or_"},
        {"orny2", "orny_"},
        {"oryn2", "oryn_"},
        {"xnor2", "xnor_"},
        {"xor2", "xor_"},
        {"lut1", "lut1"},
        {"lut2", "lut2"},
        {"lut3", "lut3"},
        {"lut4", "lut4"},
        {"lut5", "lut5"},
        {"lut6", "lut6"},
    });

static auto kCellNameToJaxitePmapOp =
    absl::flat_hash_map<absl::string_view, absl::string_view>({
        {"lut2", "jaxite_bool.pmap_lut2"},
        {"lut3", "jaxite_bool.pmap_lut3"},
    });

class JaxiteTemplates : public CodegenTemplates {
 public:
  JaxiteTemplates() = default;
  ~JaxiteTemplates() override = default;

  std::string ConstantCiphertext(int value) const override {
    return absl::StrFormat("jaxite_bool.constant(%s, params)",
                           value > 0 ? "True" : "False");
  }

  std::string PriorGateOutputReference(absl::string_view ref) const override {
    return absl::StrFormat("temp_nodes[%s]", ref);
  }

  std::string InputOrOutputReference(absl::string_view ref) const override {
    return std::string(ref);
  }
};

const JaxiteTemplates& GetJaxiteTemplates() {
  static const auto* templates = new JaxiteTemplates();
  return *templates;
}

absl::StatusOr<std::string> GateOutputAsPythonRef(
    AbstractNetRef<bool> gate_output) {
  // Some gates assign directly to outputs, and some assign to wires and rely on
  // a terminal assignment statement to get the values into the output. If a
  // gate assigns directly to the output, we need to skip the use of
  // `temp_nodes` and refer to that output index directly. Otherwise, we need to
  // parse the wire as an integer and index `temp_nodes`.
  //
  // Examples:
  //
  // "out[2]", kind==kOutput -> "out[2]"
  // "_04_" -> "temp_nodes[4]"
  std::string statement_lhs;
  if (gate_output->kind() == NetDeclKind::kOutput) {
    statement_lhs =
        GetJaxiteTemplates().InputOrOutputReference(gate_output->name());
  } else {
    // The name is a wire like `_01_` and must be parsed.
    XLS_ASSIGN_OR_RETURN(int output_name,
                         NetRefIdToNumericId(gate_output->name()));
    statement_lhs = GetJaxiteTemplates().PriorGateOutputReference(
        std::to_string(output_name));
  }
  return statement_lhs;
}

absl::StatusOr<std::string> OutputStem(
    const xls::netlist::rtl::AbstractModule<bool>& module) {
  // Each bit of each output corresponds to one entry in the `outputs` vector.
  // Hence, to validate we have only one output variable, we count the
  // occurrences of each name stem.
  absl::flat_hash_set<std::string> output_stem_names;
  for (const auto& output : module.outputs()) {
    output_stem_names.insert(NetRefStem(output->name()));
  }

  if (output_stem_names.size() != 1) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Modules with $0 outputs are not supported, names were: $1",
        output_stem_names.size(), absl::StrJoin(output_stem_names, ",")));
  }
  return std::string(*(output_stem_names.begin()));
}

constexpr absl::string_view kPrelude =
    R"(from typing import Dict, List

from jaxite.jaxite_bool import jaxite_bool
from jaxite.jaxite_lib import types

$0:$1
)";

absl::StatusOr<std::string> AddOpsInBatches(
    std::string_view batch_fn, const std::vector<AbstractCell<bool>*>& cells,
    int batch_size) {
  std::vector<std::string> statements;
  int num_batches = (int)ceil((double)cells.size() / batch_size);

  for (int batch_index = 0; batch_index < num_batches; ++batch_index) {
    absl::flat_hash_map<int, std::string> cell_index_to_output_lhs;
    std::string input_args = "  inputs = [\n";

    int remaining_gates = cells.size() - (batch_index * batch_size);
    int batch_bound = std::min(batch_size, remaining_gates);
    for (int i = 0; i < batch_bound; ++i) {
      const auto& cell = cells[batch_index * batch_size + i];
      if (cell->outputs().length() > 1) {
        return absl::InvalidArgumentError(
            "Cells with more than one output pin are not supported.");
      }
      XLS_ASSIGN_OR_RETURN(const GateInputs extracted_inputs,
                           ExtractGateInputs(cell, GetJaxiteTemplates()));
      input_args += absl::Substitute(
          "    ($0, $1),\n", absl::StrJoin(extracted_inputs.inputs, ", "),
          extracted_inputs.lut_definition);

      XLS_ASSIGN_OR_RETURN(
          std::string statement_lhs,
          GateOutputAsPythonRef(cell->outputs().front().netref));
      cell_index_to_output_lhs[i] = statement_lhs;
    }
    input_args += "  ]";
    statements.push_back(input_args);
    statements.push_back(
        absl::Substitute("  outputs = $0(inputs, sks, params)", batch_fn));

    for (int j = 0; j < batch_bound; ++j) {
      statements.push_back(absl::Substitute("  $0 = outputs[$1]",
                                            cell_index_to_output_lhs[j], j));
    }
  }
  return absl::StrJoin(statements, "\n");
}

}  // namespace

absl::StatusOr<std::string> YosysTranspiler::Translate(
    const absl::string_view cell_library_text,
    const absl::string_view netlist_text, const int parallelism) {
  XLS_ASSIGN_OR_RETURN(CharStream char_stream,
                       CharStream::FromText(std::string(cell_library_text)));
  CellLibraryProto cell_library_proto =
      *xls::netlist::function::ExtractFunctions(&char_stream);

  // The various XLS Parser classes expect a concrete EvalT, but as far as I can
  // tell this is only needed for interpretation, and we're not interpreting. So
  // the choice of EvalT and zero/one is a no-op, and we can use <bool> without
  // trouble.
  //
  // Note: we still need the cell library even though we don't use it directly,
  // because the cell library determines which pins on a cell specification
  // correspond to the input and outputs of the cell. This is used in
  // ParseNetlist and the output AbstractNetlist has structured fields for
  // input/output.
  XLS_ASSIGN_OR_RETURN(
      AbstractCellLibrary<bool> cell_library,
      AbstractCellLibrary<bool>::FromProto(cell_library_proto, false, true));

  Scanner scanner(netlist_text);
  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<AbstractNetlist<bool>> parsed_netlist_ptr,
      AbstractParser<bool>::ParseNetlist(&cell_library, &scanner, false, true));
  if (parsed_netlist_ptr->modules().length() > 1) {
    return absl::InvalidArgumentError(
        "Multiple module definitions not supported");
  }

  const AbstractModule<bool>& module = *parsed_netlist_ptr->modules().front();

  XLS_ASSIGN_OR_RETURN(std::string signature, FunctionSignature(module));
  XLS_ASSIGN_OR_RETURN(std::string function_setup, FunctionSetup(module));
  XLS_ASSIGN_OR_RETURN(std::string gate_ops,
                       parallelism > 0 ? AddParallelGateOps(module, parallelism)
                                       : AddGateOps(module));
  XLS_ASSIGN_OR_RETURN(std::string assignments, AssignOutputs(module));
  XLS_ASSIGN_OR_RETURN(std::string function_return, FunctionReturn(module));

  return absl::StatusOr<std::string>(absl::StrCat(
      absl::Substitute(kPrelude, signature, function_setup), gate_ops, "\n",
      assignments.empty() ? "" : assignments + "\n", function_return));
}

absl::StatusOr<std::string> YosysTranspiler::FunctionSetup(
    const AbstractModule<bool>& module) {
  const std::string temp_nodes_instantiation =
      "\n  temp_nodes: Dict[int, types.LweCiphertext] = {}";

  if (module.outputs().size() == 1) {
    return temp_nodes_instantiation;
  }

  XLS_ASSIGN_OR_RETURN(std::string output_stem, OutputStem(module));
  std::string output_instantiation =
      absl::StrFormat("%s = [None] * %d", output_stem, module.outputs().size());
  return absl::Substitute("$0\n  $1", temp_nodes_instantiation,
                          output_instantiation);
}

absl::StatusOr<std::string> YosysTranspiler::FunctionReturn(
    const AbstractModule<bool>& module) {
  XLS_ASSIGN_OR_RETURN(std::string output_stem, OutputStem(module));
  return absl::StrFormat("  return %s\n", output_stem);
}

// Gate instructions that form the main body of the function
absl::StatusOr<std::string> YosysTranspiler::AddGateOps(
    const AbstractModule<bool>& module) {
  std::vector<std::string> statements;

  XLS_ASSIGN_OR_RETURN(const std::vector<std::string> sorted_cell_names,
                       TopoSortedCellNames(module));
  for (const auto& cell_name : sorted_cell_names) {
    XLS_ASSIGN_OR_RETURN(const auto& cell, module.ResolveCell(cell_name));
    // In the netlist spec, for a cell evaluation statement like
    //
    //   and2 _3_ (.A(in[1]), .B(_0_), .Y(_1_));
    //
    // the string `_3_` is the `name` field of an AbstractCell, while the gate
    // operation `and2` is cell_library_entry.name. Note, however, that the wire
    // the output is connected to in the above statement is _1_, as denoted by
    // the argument to the Y pin. Hence, the _3_ is never used.
    const std::string_view gate_name = cell->cell_library_entry()->name();

    if (cell->outputs().length() > 1) {
      return absl::InvalidArgumentError(
          "Cells with more than one output pin are not supported.");
    }

    XLS_ASSIGN_OR_RETURN(const GateInputs extracted_inputs,
                         ExtractGateInputs(cell, GetJaxiteTemplates()));
    std::vector<std::string> gate_inputs = extracted_inputs.inputs;

    // the jaxite_bool.not_ does not have an `sks` arg.
    std::string_view param_args = gate_name == "inv" ? "params" : "sks, params";

    XLS_ASSIGN_OR_RETURN(std::string statement_lhs,
                         GateOutputAsPythonRef(cell->outputs().front().netref));

    if (gate_name == kBufferGateName) {
      if (gate_inputs.size() > 1) {
        return absl::InvalidArgumentError(
            absl::StrFormat("Buffer cells must have exactly one input, but "
                            "found one with %d inputs.",
                            gate_inputs.size()));
      }
      statements.push_back(
          absl::Substitute("  $0 = $1", statement_lhs, gate_inputs[0]));
    } else {
      if (!kCellNameToJaxiteOp.contains(gate_name)) {
        return absl::InvalidArgumentError(absl::StrFormat(
            "Found cell %s with unknown codegen mapping", gate_name));
      }
      if (absl::StrContains(gate_name, "lut")) {
        statements.push_back(absl::Substitute(
            "  $0 = jaxite_bool.$1($2, $3, $4)", statement_lhs,
            kCellNameToJaxiteOp.at(gate_name), absl::StrJoin(gate_inputs, ", "),
            extracted_inputs.lut_definition, param_args));
      } else {
        statements.push_back(
            absl::Substitute("  $0 = jaxite_bool.$1($2, $3)", statement_lhs,
                             kCellNameToJaxiteOp.at(gate_name),
                             absl::StrJoin(gate_inputs, ", "), param_args));
      }
    }
  }

  return absl::StrJoin(statements, "\n");
}

// Gate instructions that form the main body of the function. Same as
// AddGateOpps, but split cells by gate and uses an appropriate parallel op in
// batches of up to gate_parallelism size.
absl::StatusOr<std::string> YosysTranspiler::AddParallelGateOps(
    const AbstractModule<bool>& module, int gate_parallelism) {
  std::vector<std::string> statements;
  XLS_LOG(INFO) << "Generating parallel gate ops with parallelism "
                << gate_parallelism << "\n";

  XLS_ASSIGN_OR_RETURN(std::vector<std::vector<std::string>> cell_levels,
                       LevelSortedCellNames(module));
  for (int level_index = 0; level_index < cell_levels.size(); ++level_index) {
    auto& level = cell_levels[level_index];

    // Sorting is not strictly necessary for correctness, because all nodes in a
    // level can be evaluated independently, but it's here to ensure the test
    // outputs are consistent and deterministic.
    std::sort(level.begin(), level.end());

    // Use an std::map so order of keys is consistent for the same reason
    auto by_gate = std::map<std::string, std::vector<AbstractCell<bool>*>>();
    for (const auto& cell_name : level) {
      XLS_ASSIGN_OR_RETURN(const auto& cell, module.ResolveCell(cell_name));
      auto gate = cell->cell_library_entry()->name();
      by_gate[gate].push_back(cell);
    }

    for (const auto& [gate_name, cells] : by_gate) {
      if (!kCellNameToJaxitePmapOp.contains(gate_name)) {
        return absl::InvalidArgumentError(absl::StrFormat(
            "Found cell %s with unknown pmap codegen mapping", gate_name));
      }
      auto batch_fn = kCellNameToJaxitePmapOp.at(gate_name);
      XLS_ASSIGN_OR_RETURN(auto gate_statements,
                           AddOpsInBatches(batch_fn, cells, gate_parallelism));
      statements.push_back(gate_statements);
    }
  }
  return absl::StrJoin(statements, "\n");
}

// Statements that copy internal temp_nodes to the output locations.
absl::StatusOr<std::string> YosysTranspiler::AssignOutputs(
    const AbstractModule<bool>& module) {
  std::vector<std::string> assignments;

  for (const auto& [key, value] : module.assigns()) {
    // The variable being assigned to must be an output of the module, although
    // this is not a required part of the netlist language spec, the XLS netlist
    // parser seems to assume this by converting the assignment statements to a
    // map that, in particular, drops the ordering of assignment statements.
    if (key->kind() != NetDeclKind::kOutput) {
      return absl::InvalidArgumentError(
          "Unsupported assign statement assigning to non-output variables.");
    }

    auto templates = GetJaxiteTemplates();
    std::string var_value;
    if (absl::StrContains(value->name(), "constant")) {
      XLS_ASSIGN_OR_RETURN(int constant_value, ConstantToValue(value->name()));
      var_value = templates.ConstantCiphertext(constant_value);
    } else {
      XLS_ASSIGN_OR_RETURN(var_value, ResolveNetRefName(value, templates));
    }

    XLS_ASSIGN_OR_RETURN(const std::string variable,
                         ResolveNetRefName(key, GetJaxiteTemplates()));
    assignments.push_back(absl::Substitute("  $0 = $1", variable, var_value));
  }

  // Since map iteration order is arbitrary, and the assignments to outputs are
  // supposed to be unique, we can sort the output and get a deterministic
  // output program.
  std::sort(assignments.begin(), assignments.end());
  return absl::StrJoin(assignments, "\n");
}

absl::StatusOr<std::string> YosysTranspiler::FunctionSignature(
    const xls::netlist::rtl::AbstractModule<bool>& module) {
  // Each entry in module.inputs() corresponds to one bit of an input, though
  // many inputs may share the same stem. However, if there is only a single bit
  // in the input, it will be referred to by just the stem (e.g., `value` vs
  // `value[3]`), and it requires a different type in the function signature: a
  // single value instead of a list of values. We count the occurrences of the
  // stem (`value` for `value[3]`) in order to determine if it's a single bit or
  // multi-bit input.
  absl::flat_hash_map<std::string, int> input_stem_counts;
  for (const auto& input : module.inputs()) {
    input_stem_counts[NetRefStem(input->name())] += 1;
  }

  int output_size = module.outputs().size();
  std::string output_type =
      output_size == 1 ? "types.LweCiphertext" : "List[types.LweCiphertext]";

  std::vector<std::string> param_signatures;
  for (const auto& input : module.inputs()) {
    std::string input_stem = NetRefStem(input->name());
    if (!input_stem_counts.count(input_stem)) {
      continue;
    }
    int stem_count = input_stem_counts[input_stem];
    // We want to process this stem exactly once, so that only one argument is
    // added for each possible stem.
    input_stem_counts.erase(input_stem);
    if (stem_count == 1) {
      param_signatures.push_back(
          absl::StrCat(input_stem, ": types.LweCiphertext"));
    } else {
      param_signatures.push_back(
          absl::StrCat(input_stem, ": List[types.LweCiphertext]"));
    }
  }

  return absl::Substitute(
      "def $0($1, sks: jaxite_bool.ServerKeySet, params: "
      "jaxite_bool.Parameters) -> $2",
      ToSnakeCase(module.name()), absl::StrJoin(param_signatures, ", "),
      output_type);
}

}  // namespace transpiler
}  // namespace jaxite
}  // namespace fhe
