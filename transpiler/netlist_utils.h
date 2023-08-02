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

// Helper functions for Boolean YoSys Transpiler.

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_NETLIST_UTILS_H
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_NETLIST_UTILS_H

#include <functional>
#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "xls/netlist/netlist.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

absl::StatusOr<::xls::netlist::AbstractCellLibrary<bool>> ParseCellLibrary(
    const absl::string_view cell_library_text);

absl::StatusOr<std::unique_ptr<::xls::netlist::rtl::AbstractNetlist<bool>>>
ParseNetlist(::xls::netlist::AbstractCellLibrary<bool>& cell_library,
             const absl::string_view netlist_text);

// Returns the list of cell names in topologically sorted order, or return an
// error if the netlist is cyclic. An error should be impossible for FHE
// programs, since they are represented as pure combinational circuits.
absl::StatusOr<std::vector<std::string>> TopoSortedCellNames(
    const xls::netlist::rtl::AbstractModule<bool>& module);

// Returns a vector of vectors of node names, where each vector represents
// a different level in the graph. Nodes in the same level can be executed in
// parallel.
// Returns an error if the netlist is cyclic. An error should be impossible for
// FHE programs, since they are represented as pure combinational circuits.
absl::StatusOr<std::vector<std::vector<std::string>>> LevelSortedCellNames(
    const xls::netlist::rtl::AbstractModule<bool>& module);

// The set of inputs to a gate, along with a truth table, if the gate contains a
// truth table hard coded among its input gates.
struct GateInputs {
  std::vector<std::string> inputs;
  uint64_t lut_definition;
};

// Data describing the output of a cell.
struct GateOutput {
  // The full name of the output netref, e.g., `out`, `out[4]`, or `_5_`
  std::string name;

  // Whether the output is part of a netref with more than one bit.
  // E.g., for `out[4]` this field is true, but for `out` and `_5_`
  // it is false.
  bool is_single_bit;

  // The integer part of the full name, is_single_bit is false
  int index;

  // True if the output corresponds to the module's output net.
  bool is_output;
};

// The template functions for handling language-specific constructions
// E.g., in a Python backend a gate output is stored to temp_nodes[$0],
// but in another backend it will have a different syntax.
class CodegenTemplates {
 public:
  virtual ~CodegenTemplates() = default;

  // A function that maps a constant, public integer to the corresponding
  // ciphertext. E.g., it might map 1 -> 'jaxite_bool.constant(True, params)'
  virtual std::string ConstantCiphertext(int value) const = 0;

  // A function that resolves an expression reference to a prior gate output,
  // such as the input "4" representing the value on wire _4_.
  virtual std::string PriorGateOutputReference(absl::string_view ref) const = 0;

  // A function that resolves an expression reference to an input or output
  // netref, like my_input[0]
  virtual std::string InputOrOutputReference(absl::string_view ref) const = 0;
};

// Convert a netlist cell identifier `_\d+_` into the numeric part of the
// identifier.
absl::StatusOr<int> NetRefIdToNumericId(absl::string_view netref_id);

// Convert a netlist cell identifier `foo[\d+]` into the numeric part of the
// index. If there is none, default to 0.
absl::StatusOr<int> NetRefIdToIndex(absl::string_view netref_id);

// Get the part of a string like `foo[7]` before the first `[`.
// Since some inputs and outputs can be single-bits, the `[7]` is also optional
std::string NetRefStem(std::string_view netref);

// Convert a string like <constant_5> to the integer 5.
absl::StatusOr<int> ConstantToValue(std::string_view constant);

// Convert a net reference into the corresponding language-specific
// backend.
absl::StatusOr<std::string> ResolveNetRefName(
    const xls::netlist::rtl::AbstractNetRef<bool>& netref,
    const CodegenTemplates& templates);

// Returns the set of gate inputs to a given gate.
// Example 1 (not a LUT):
// Input: imux2 _3_ (.A(_0_), .B(_1_), .S(_2_), .Y(_3_))
// Output: {Inputs: "0", "1", "2", lut_definition=0}
//
// Example 2 (a LUT):
// Input: lut2 _11_ (.A(_02_), .B(x[7]),
//                   .P0(1'h1), .P1(1'h0), .P2(1'h1), .P3(1'h1), .Y(out[7]));
// Output: {Inputs: "temp_nodes[2]", "x[7]", lut_definition=13}
//                                                          ^ little endian
//
// Note, some of these gates are not symmetric in their inputs. This
// implementation assumes that the order in which the inputs are parsed is
// the same as the order in which they occur in the liberty cell spec.
// I.e., for this cell definition:
//
//   cell(imux2) {
//     pin(A) { direction : input; }
//     pin(B) { direction : input; }
//     pin(S) { direction : input; }
//     pin(Y) { function : "(A * S) + (B * (S'))"; }
//   }
//
// it is critical that the order of A, B, S are unchanged
// at the callsite.
//
//   imux2 _3_ (.A(_0_), .B(_1_), .S(_2_), .Y(_3_));
//
// Circuits output by Yosys appear to uphold this guarantee in their output,
// and the XLS netlist parser appears to preserve this after parsing.
//
// Note that to avoid this assumption, we would need to also process the
// liberty file cell definition, and somehow connect the symbols used in its
// input to the corresponding inputs to give to jaxite_bool's functions.
// This is a lot more work.
absl::StatusOr<GateInputs> ExtractGateInputs(
    const xls::netlist::rtl::AbstractCell<bool>* cell,
    const CodegenTemplates& templates);

// The same as the above method, but only extracts the numeric ids for the
// inputs to this cell that correspond to the outputs of previously evaluated
// cells.
absl::StatusOr<std::vector<int>> ExtractPriorGateOutputIds(
    const xls::netlist::rtl::AbstractCell<bool>* cell);

// Extract the output data for a single cell
absl::StatusOr<GateOutput> ExtractGateOutput(
    const xls::netlist::rtl::AbstractCell<bool>* cell);

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_NETLIST_UTILS_H
