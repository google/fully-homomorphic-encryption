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

#include "transpiler/yosys_tfhe_runner.h"

#include "absl/container/flat_hash_map.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/text_format.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/netlist/netlist.h"
#include "xls/public/value.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

using EvalFn = xls::netlist::rtl::CellOutputEvalFn<LweSample>;

// NOTE: The input order to methods YosysTfheRunner::TfheOp_* is the same as the
// order in which the pins are declared in the Liberty file.  This is mostly as
// you expect, but note that in the case of imux2, the pin order is "A", "B",
// and "Y", with "Y" being the output, while bootsMUX, which handles the
// operation, expects the control ("Y") to come first, then the inputs "A" and
// "B".

#define IMPL1(cell, OP)                                         \
  absl::StatusOr<TfheBoolValue> YosysTfheRunner::TfheOp_##cell( \
      const std::vector<TfheBoolValue>& args) {                 \
    XLS_CHECK(args.size() == 1);                                \
    LweSample* result =                                         \
        new_gate_bootstrapping_ciphertext(state_->bk_->params); \
    boots##OP(result, args[0].lwe().get(), state_->bk_);        \
    return TfheBoolValue(result, state_->bk_);                  \
  }

#define IMPL2(cell, OP)                                                       \
  absl::StatusOr<TfheBoolValue> YosysTfheRunner::TfheOp_##cell(               \
      const std::vector<TfheBoolValue>& args) {                               \
    XLS_CHECK(args.size() == 2);                                              \
    LweSample* result =                                                       \
        new_gate_bootstrapping_ciphertext(state_->bk_->params);               \
    boots##OP(result, args[0].lwe().get(), args[1].lwe().get(), state_->bk_); \
    return TfheBoolValue(result, state_->bk_);                                \
  }

IMPL1(inv, NOT);
IMPL1(buffer, COPY);
IMPL2(and2, AND);
IMPL2(nand2, NAND);
IMPL2(or2, OR);
IMPL2(andyn2, ANDYN);
IMPL2(andny2, ANDNY);
IMPL2(oryn2, ORYN);
IMPL2(orny2, ORNY);
IMPL2(nor2, NOR);
IMPL2(xor2, XOR);
IMPL2(xnor2, XNOR);
#undef IMPL1
#undef IMPL2

absl::StatusOr<TfheBoolValue> YosysTfheRunner::TfheOp_imux2(
    const std::vector<TfheBoolValue>& args) {
  XLS_CHECK(args.size() == 3);
  LweSample* result = new_gate_bootstrapping_ciphertext(state_->bk_->params);
  bootsMUX(result, args[2].lwe().get(), args[0].lwe().get(),
           args[1].lwe().get(), state_->bk_);
  return TfheBoolValue(result, state_->bk_);
}

absl::Status YosysTfheRunner::Run(
    absl::Span<LweSample> result,
    std::vector<absl::Span<const LweSample>> in_args,
    std::vector<absl::Span<LweSample>> inout_args,
    const TFheGateBootstrappingCloudKeySet* bk) {
#define OP(name)                                           \
  {                                                        \
    #name, {                                               \
      {                                                    \
        "Y",                                               \
            [this](const std::vector<TfheBoolValue>& args) \
                -> absl::StatusOr<TfheBoolValue> {         \
              return this->TfheOp_##name(args);            \
            }                                              \
      }                                                    \
    }                                                      \
  }
  if (state_ == nullptr) {
    xls::netlist::rtl::CellToOutputEvalFns<TfheBoolValue> tfhe_eval_map{
        OP(inv),    OP(buffer), OP(and2),  OP(nand2), OP(or2),
        OP(andyn2), OP(andny2), OP(oryn2), OP(orny2), OP(nor2),
        OP(xor2),   OP(xnor2),  OP(imux2),
    };

    XLS_RETURN_IF_ERROR(InitializeOnce(bk, tfhe_eval_map));
  }
#undef OP

  return state_->Run(result, in_args, inout_args);
}

absl::Status YosysTfheRunner::InitializeOnce(
    const TFheGateBootstrappingCloudKeySet* bk,
    const xls::netlist::rtl::CellToOutputEvalFns<TfheBoolValue>& eval_fns) {
  if (state_ == nullptr) {
    state_ = std::make_unique<YosysTfheRunnerState>(
        bk, *xls::netlist::cell_lib::CharStream::FromText(liberty_text_),
        xls::netlist::rtl::Scanner(netlist_text_));

    state_->netlist_ = std::move(
        *xls::netlist::rtl::AbstractParser<TfheBoolValue>::ParseNetlist(
            &state_->cell_library_, &state_->scanner_, state_->zero_,
            state_->one_));

    XLS_RETURN_IF_ERROR(state_->netlist_->AddCellEvaluationFns(eval_fns));

    XLS_CHECK(google::protobuf::TextFormat::ParseFromString(
        metadata_text_, &state_->metadata_));
  }
  return absl::OkStatus();
}

absl::Status YosysTfheRunner::YosysTfheRunnerState::Run(
    absl::Span<LweSample> result,
    std::vector<absl::Span<const LweSample>> in_args,
    std::vector<absl::Span<LweSample>> inout_args) {
  std::string function_name = metadata_.top_func_proto().name().name();
  XLS_ASSIGN_OR_RETURN(auto module, netlist_->GetModule(function_name));

  // Arguments are in the form of spans of LweSamples, with one span per input
  // argument.  So for example, if you have two inputs, an uint8_t and an
  // int32_t, then you'll have two spans, the first of which is 8-bits wide, and
  // the second of which is 32-bits wide.  Each span entry will be a LweSample
  // representing one bit of the input.  It represents a bit of input, but it
  // does not act like a "bool".
  //
  // The interpreter, on the other hand, expects values that act as booleans:
  // they can be constructed from bool and can participate in boolean operators.
  // From the interpreter's view, they or may not be evaluated to booleans (in
  // our case, or course, we want to prevent such evaluation).
  //
  //    a) Constructing from bool is necessary to assign initial values to
  //    constants in the netlist.  These constants are part of the algorithm,
  //    and constructing TFHE objects from them is OK.  Also, technically
  //    evaluating them as bool is OK since we know their values already.
  //
  //    b) Bool expressions.  The Interpreter has code to either parse *and*
  //    interpret the cell-output-pin-function definitions already provided in
  //    the cell library as part of the cell definitions, or to parse the
  //    functions but trap directly into our callback implementations (e.g.,
  //    YosysTfheRunner::TfheOp_xor2) to do the actual evaluation.  For the
  //    latter, we only need requirement (a) above, because all the actual
  //    operations are handled in the callbacks.  However, the Interpreter is
  //    coded to handle the case where a callback isn't available, and so it
  //    needs to be able to evaluate the FHE objects as booleans as usual.  For
  //    this reason, we must provide arithmetic operation capabilities to our
  //    FHE booleans.
  //
  //    c) No evaluation to bool.  For obvious reasons.
  //
  // Requirement (b) is useful if for some reasons we cannot provide an
  // implementation of a cell and instead rely on the function parser to
  // interpret it.  In the extreme case, we can simply not pass TfheEvalMap to
  // the interpreter, forcing it to evaluate everything.  That will still work
  // since the FHE objects act as bools.

  using NetRef = xls::netlist::rtl::AbstractNetRef<TfheBoolValue>;
  std::vector<TfheBoolValue> input_bits;
  size_t in_i = 0, inout_i = 0;
  for (const auto& param : metadata_.top_func_proto().params()) {
    std::vector<TfheBoolValue> arg_bits;
    if (param.is_reference() && !param.is_const()) {
      XLS_CHECK(inout_i < inout_args.size());
      const auto& arg = inout_args[inout_i++];
      arg_bits.reserve(arg.size());
      for (int i = 0; i < arg.size(); i++) {
        arg_bits.emplace_back(arg.data() + i, bk_);
      }
    } else {
      XLS_CHECK(in_i < in_args.size());
      const auto& arg = in_args[in_i++];
      arg_bits.reserve(arg.size());
      for (int i = 0; i < arg.size(); i++) {
        arg_bits.emplace_back(arg.data() + i, bk_);
      }
    }
    input_bits.insert(input_bits.begin(), arg_bits.begin(), arg_bits.end());
  }
  std::reverse(input_bits.begin(), input_bits.end());

  xls::netlist::AbstractNetRef2Value<TfheBoolValue> input_nets;
  const std::vector<NetRef>& module_inputs = module->inputs();
  XLS_CHECK(module_inputs.size() == input_bits.size());

  for (int i = 0; i < module->inputs().size(); i++) {
    const NetRef in = module_inputs[i];
    XLS_CHECK(!input_nets.contains(in));
    input_nets.emplace(
        in, std::move(input_bits[module->GetInputPortOffset(in->name())]));
  }

  TfheBoolValue zero(false, bk_);
  TfheBoolValue one(true, bk_);
  // *2 for hyperthreading opportunities
  const int num_threads = sysconf(_SC_NPROCESSORS_ONLN) * 2;
  xls::netlist::AbstractInterpreter<TfheBoolValue> interpreter(
      netlist_.get(), zero, one, num_threads);
  XLS_ASSIGN_OR_RETURN(auto output_nets,
                       interpreter.InterpretModule(module, input_nets, {}));

  // The return value output_nets is a map from NetRef to TfheBoolValue objects.
  // Each of the TfheBoolValue objects contains a LweSample*, which it either
  // owns or has borrowed from elsewhere (whether it owns or has borrowed does
  // not matter here.)
  //
  // We need to map the output_nets-contained LweSamples to the result. We do
  // that by assigning each pointer in the result array to the corresponding
  // pointer in the output_nets-owned LweSamples.

  std::vector<std::shared_ptr<LweSample>> output_bit_vector;

  XLS_CHECK(module->outputs().size() == output_nets.size());
  for (const NetRef ref : module->outputs()) {
    auto tfhe_bool = output_nets.at(ref);
    auto lwe = tfhe_bool.lwe();
    XLS_CHECK(lwe != nullptr);
    output_bit_vector.push_back(lwe);
  }

  // As we iterate over output_bit_vector, we'll use this iterator.
  auto out = output_bit_vector.cbegin();
  size_t copied = 0;

  // The output_nets are bits with (return value, set of in_out args) splayed
  // out in the reverse order (due to verilog endianness).

  // First, in_out args from the output bits are copied over(in reverse order).
  int params_inout_idx = inout_args.size() - 1;
  for (const auto& param : metadata_.top_func_proto().params()) {
    if (param.is_reference() && !param.is_const()) {
      LweSample* lwe = inout_args[params_inout_idx].data();
      for (int i = 0; i < inout_args[params_inout_idx].size(); i++) {
        bootsCOPY(&lwe[i], out->get(), bk_);
        out++;
        copied++;
      }
      params_inout_idx--;
    }
  }

  // The return value of the function now comes last, so we copy that.
  // If there is no return value, then result.size() == 0 and we do not copy
  // anything.
  for (int i = 0; i < result.size(); i++, out++, copied++) {
    LweSample* lwe = result.data();
    bootsCOPY(&lwe[i], out->get(), bk_);
  }

  XLS_CHECK(copied == output_bit_vector.size());
  XLS_CHECK(out == output_bit_vector.cend());

  return absl::OkStatus();
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
