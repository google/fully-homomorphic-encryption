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

#include "transpiler/yosys_cleartext_runner.h"

#include <algorithm>

#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/public/value.h"

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

    state_->netlist_ =
        std::move(*xls::netlist::rtl::AbstractParser<BoolValue>::ParseNetlist(
            &state_->cell_library_, &state_->scanner_, state_->zero_,
            state_->one_));

    XLS_RETURN_IF_ERROR(state_->netlist_->AddCellEvaluationFns(eval_fns));

    XLS_CHECK(google::protobuf::TextFormat::ParseFromString(
        metadata_text_, &state_->metadata_));
  }
  return absl::OkStatus();
}

absl::Status YosysRunner::Run(
    absl::Span<OpaqueValue> result,
    std::vector<absl::Span<const OpaqueValue>> in_args,
    std::vector<absl::Span<OpaqueValue>> inout_args) {
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

  return state_->Run(result, in_args, inout_args);
}

absl::Status YosysRunner::YosysRunnerState::Run(
    absl::Span<OpaqueValue> result,
    std::vector<absl::Span<const OpaqueValue>> in_args,
    std::vector<absl::Span<OpaqueValue>> inout_args) {
  std::string function_name = metadata_.top_func_proto().name().name();
  XLS_ASSIGN_OR_RETURN(auto module, netlist_->GetModule(function_name));

  xls::Bits input_bits;
  size_t in_i = 0, inout_i = 0;
  for (const auto& param : metadata_.top_func_proto().params()) {
    if (param.is_reference() && !param.is_const()) {
      XLS_CHECK(inout_i < inout_args.size());
      xls::Bits arg_bits(inout_args[inout_i++]);
      input_bits = xls::bits_ops::Concat({input_bits, arg_bits});
    } else {
      XLS_CHECK(in_i < in_args.size());
      xls::Bits arg_bits(in_args[in_i++]);
      input_bits = xls::bits_ops::Concat({input_bits, arg_bits});
    }
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

  BoolValue zero{OpaqueValue{false}};
  BoolValue one{OpaqueValue{true}};
  xls::netlist::AbstractInterpreter<OpaqueValue> interpreter(netlist_.get(),
                                                             zero, one);
  XLS_ASSIGN_OR_RETURN(auto output_nets,
                       interpreter.InterpretModule(module, input_nets, {}));

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
  size_t copied = 0;

  // The output_nets are bits with (return value, set of in_out args) splayed
  // out in the reverse order (due to verilog endianness).

  // First, in_out args from the output bits are copied over(in reverse order).
  int params_inout_idx = inout_args.size() - 1;
  for (const auto& param : metadata_.top_func_proto().params()) {
    if (param.is_reference() && !param.is_const()) {
      std::copy_n(out, inout_args[params_inout_idx].size(),
                  inout_args[params_inout_idx].begin());
      out += inout_args[params_inout_idx].size();
      copied += inout_args[params_inout_idx].size();
      params_inout_idx--;
    }
  }

  // The return value of the function now comes last, so we copy that.
  // If there is no return value, then result.size() == 0 and we do not copy
  // anything.
  std::copy_n(out, result.size(), result.begin());
  out += result.size();
  copied += result.size();

  XLS_CHECK(copied == output_bit_vector.size());
  XLS_CHECK(out == output_bit_vector.cend());

  return absl::OkStatus();
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
