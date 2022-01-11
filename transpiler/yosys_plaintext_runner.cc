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

#include "transpiler/yosys_plaintext_runner.h"

#include "absl/strings/substitute.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/ir/bits.h"
#include "xls/ir/bits_ops.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

using NetRef = xls::netlist::rtl::AbstractNetRef<OpaqueValue>;

using EvalFn = xls::netlist::rtl::CellOutputEvalFn<OpaqueValue>;

// NOTE: The input order to methods YosysTfheRunner::TfheOp_* is the same as the
// order in which the pins are declared in the Liberty file.  This is mostly as
// you expect, but note that in the case of imux2, the pin order is "A", "B",
// and "Y", with "Y" being the output, while bootsMUX, which handles the
// operation, expects the control ("Y") to come first, then the inputs "A" and
// "B".

#define IMPL1(cell, OP)                                 \
  absl::StatusOr<BoolValue> YosysRunner::TfheOp_##cell( \
      const std::vector<BoolValue>& args) {             \
    XLS_CHECK(args.size() == 1);                        \
    OpaqueValue result = OP(args[0]);                   \
    return BoolValue(result);                           \
  }

#define IMPL2(cell, OP)                                 \
  absl::StatusOr<BoolValue> YosysRunner::TfheOp_##cell( \
      const std::vector<BoolValue>& args) {             \
    XLS_CHECK(args.size() == 2);                        \
    OpaqueValue result = OP(args[0], args[1]);          \
    return BoolValue(result);                           \
  }

IMPL1(inv, [](auto a) { return !a; });
IMPL1(buffer, [](auto a) { return a; });
IMPL2(and2, [](auto a, auto b) { return a & b; });
IMPL2(nand2, [](auto a, auto b) { return !(a & b); });
IMPL2(or2, [](auto a, auto b) { return a | b; });
IMPL2(andyn2, [](auto a, auto b) { return a & !b; });
IMPL2(andny2, [](auto a, auto b) { return !a & b; });
IMPL2(oryn2, [](auto a, auto b) { return a | !b; });
IMPL2(orny2, [](auto a, auto b) { return !a | b; });
IMPL2(nor2, [](auto a, auto b) { return !(a | b); });
IMPL2(xor2, [](auto a, auto b) { return a ^ b; });
IMPL2(xnor2, [](auto a, auto b) { return !(a ^ b); });
#undef IMPL1
#undef IMPL2

absl::StatusOr<BoolValue> YosysRunner::TfheOp_imux2(
    const std::vector<BoolValue>& args) {
  XLS_CHECK(args.size() == 3);
  OpaqueValue result = (args[0] & args[2]) | (args[1] & !args[2]);
  return BoolValue(result);
}

absl::Status YosysRunner::InitializeOnce(
    const xls::netlist::rtl::CellToOutputEvalFns<BoolValue>& eval_fns) {
  if (state_ == nullptr) {
    state_ = std::make_unique<YosysRunnerState>(
        *xls::netlist::cell_lib::CharStream::FromText(liberty_text_),
        xls::netlist::rtl::Scanner(netlist_text_));

    std::cout << "Parsing netlist..." << std::endl;
    auto start_time = absl::Now();
    state_->netlist_ =
        std::move(*xls::netlist::rtl::AbstractParser<BoolValue>::ParseNetlist(
            &state_->cell_library_, &state_->scanner_, state_->zero_,
            state_->one_));
    std::cout << "Parsing netlist took "
              << absl::ToDoubleSeconds(absl::Now() - start_time) << " secs"
              << std::endl;

    std::cout << "Adding cell-evaluation functions." << std::endl;
    XLS_RETURN_IF_ERROR(state_->netlist_->AddCellEvaluationFns(eval_fns));
    std::cout << "Done adding cell-evaluation functions." << std::endl;

    std::cout << "Parsing metadata." << std::endl;
    XLS_CHECK(google::protobuf::TextFormat::ParseFromString(
        metadata_text_, &state_->metadata_));
    std::cout << "Done parsing metadata." << std::endl;
  }
  return absl::OkStatus();
}

absl::Status YosysRunner::Run(absl::Span<OpaqueValue> result,
                              std::vector<absl::Span<OpaqueValue>> args) {
#define OP(name)                                       \
  {                                                    \
#name, {                                           \
      {                                                \
        "Y",                                           \
            [this](const std::vector<BoolValue>& args) \
                -> absl::StatusOr<BoolValue> {         \
              return this->TfheOp_##name(args);        \
            }                                          \
      }                                                \
    }                                                  \
  }
  if (state_ == nullptr) {
    xls::netlist::rtl::CellToOutputEvalFns<BoolValue> tfhe_eval_map{
        OP(inv),    OP(buffer), OP(and2),  OP(nand2), OP(or2),
        OP(andyn2), OP(andny2), OP(oryn2), OP(orny2), OP(nor2),
        OP(xor2),   OP(xnor2),  OP(imux2),
    };

    XLS_RETURN_IF_ERROR(InitializeOnce(tfhe_eval_map));
  }
#undef OP

  return state_->Run(result, args);
}

absl::Status YosysRunner::YosysRunnerState::Run(
    absl::Span<OpaqueValue> result, std::vector<absl::Span<OpaqueValue>> args) {
  std::cout << "Setting up inputs." << std::endl;
  std::string function_name = metadata_.top_func_proto().name().name();
  XLS_ASSIGN_OR_RETURN(auto module, netlist_->GetModule(function_name));

  xls::Bits input_bits;
  for (auto arg : args) {
    xls::Bits arg_bits(arg);
    input_bits = xls::bits_ops::Concat({input_bits, arg_bits});
  }
  input_bits = xls::bits_ops::Reverse(input_bits);

  xls::netlist::AbstractNetRef2Value<OpaqueValue> input_nets;
  const std::vector<NetRef>& module_inputs = module->inputs();
  XLS_CHECK(module_inputs.size() == input_bits.bit_count());

  for (int i = 0; i < module->inputs().size(); i++) {
    const NetRef in = module_inputs[i];
    XLS_CHECK(!input_nets.contains(in));
    input_nets[in] = input_bits.Get(module->GetInputPortOffset(in->name()));
  }

  std::cout << "Interpreting module." << std::endl;
  BoolValue zero{OpaqueValue{false}};
  BoolValue one{OpaqueValue{true}};
  xls::netlist::AbstractInterpreter<OpaqueValue> interpreter(netlist_.get(),
                                                             zero, one);
  XLS_ASSIGN_OR_RETURN(auto output_nets,
                       interpreter.InterpretModule(module, input_nets, {}));

  std::cout << "Collecting outputs." << std::endl;
  // Time to map the outputs returned by the netlist interpreter to the outputs
  // and in/out parameters of the source function.  We start by converting the
  // output nets to output_bit_vector--a vector of individual bit values.
  xls::BitsRope rope(output_nets.size());
  for (const NetRef ref : module->outputs()) {
    rope.push_back(output_nets[ref]);
  }
  xls::Bits output_bits = rope.Build();
  auto output_bit_vector = output_bits.ToBitVector();

  // As we iterate over output_bit_vector, we'll use this iterator.
  auto out = output_bit_vector.cbegin();

  // The return value of the function will always come first, so we copy that.
  // If there is no return value, then result.size() == 0 and we do not copy
  // anything.
  std::copy_n(out, result.size(), result.begin());

  out += result.size();

  // The remaining output wires in the netlist follow the declaration order of
  // the input wires in the verilog file.  Suppose you have the following
  // netlist:
  //
  // module foo(a, b, c, out);
  //     input [7:0] c;
  //     input a;
  //     output [7:0] out;
  //     input [7:0] b;
  //
  // Suppose that in that netlist input wires a, b, and c represent in/out
  // parameters in the source language (i.e. they are non-const references in
  // C++).
  //
  // In this case, the return values of these parameters will be splayed out in
  // the ouput in the same order in which the input wires are declared, rather
  // than the order in which they appear in the module statement.  In other
  // words, at this point out will have c[0], c[1], ... c[7], then it will have
  // a, and finally it will have b[0], ..., b[7].
  //
  // However, the inputs to the runner follow the order in the module statement
  // (which in turn mirrors the order in which they are in the source language.)
  // Therefore, we have to identify which parts of the output wires correspond
  // to each of the input arguments.
  size_t copied = result.size();
  for (int i = 0; i < module_inputs.size(); i++) {
    // Start by pulling off the first input net wire.  Following the example
    // above, it will have the name "c[0]".  (When we get to the single-wire "a"
    // next, the name will simply be "a".).
    std::vector<std::string> name_and_idx =
        absl::StrSplit(module_inputs[i]->name(), '[');
    // Look for the non-indexed name ("c" in the example above) in the list of
    // function arguments, which we can access from the metadata.
    auto found = std::find_if(
        metadata_.top_func_proto().params().cbegin(),
        metadata_.top_func_proto().params().cend(),
        [&name_and_idx](const xlscc_metadata::FunctionParameter& arg) {
          return arg.name() == name_and_idx[0];
        });
    // We must be able to find that parameter--failing to is a bug, as we
    // autogenerate both the netlist and the metadata from the same source file,
    // and provide these parameters to this method.
    XLS_CHECK(found != metadata_.top_func_proto().params().cend());

    if (found->is_reference() && !found->is_const()) {
      // Find the index of the argument for our match.  In our example, it will
      // be 2, since args[2] is the span for the encoded form of argument "c".
      size_t params_i =
          std::distance(metadata_.top_func_proto().params().begin(), found);
      // Get the bit size of the argument (e.g., 8 since "c" is defined to be a
      // byte.)
      size_t arg_size = args[params_i].size();

      // Now read out the index of the parameter itself (e.g., the 0 in "c[0]").
      size_t params_i_idx = 0;
      if (name_and_idx.size() == 2) {
        absl::string_view idx = absl::StripSuffix(name_and_idx[1], "]");
        XLS_CHECK(absl::SimpleAtoi(idx, &params_i_idx));
      }
      // The i'th parameter subscript must be within range (e.g., 0 must be less
      // than 8, since c is a byte in out example.)
      XLS_CHECK(params_i_idx < arg_size);
      // Now, the out[i] is the return value for c[0] from the example above.
      // More generally, out[i] is the write-back value of the param_i'th
      // argument at index param_i_idx.  Copy that bit directly into the output.
      // In our example, this represents argument "c" at index 0, which is
      // exactly args[2].
      const auto* src = (out + i);
      auto* dest = args[params_i].data() + params_i_idx;
      *dest = *src;
      copied++;
    }
  }
  XLS_CHECK(copied == output_bit_vector.size());

  std::cout << "Done." << std::endl;

  return absl::OkStatus();
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
