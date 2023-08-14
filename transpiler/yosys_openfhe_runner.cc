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

#include "transpiler/yosys_openfhe_runner.h"

#include <algorithm>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/text_format.h"
#include "openfhe/binfhe/binfhecontext.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/protected/netlist.h"
#include "xls/public/value.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

using EvalFn = xls::netlist::rtl::CellOutputEvalFn<lbcrypto::LWECiphertext>;

// NOTE: The input order to methods YosysOpenFheRunner::OpenFheOp_* is the
// same as the order in which the pins are declared in the Liberty file.  This
// is generally as you expect.

absl::StatusOr<OpenFheBoolValue> YosysOpenFheRunner::OpenFheOp_inv(
    const std::vector<OpenFheBoolValue>& args) {
  XLS_CHECK_EQ(args.size(), 1);
  return OpenFheBoolValue(state_->cc_.EvalNOT(args[0].lwe()), state_->cc_);
}

absl::StatusOr<OpenFheBoolValue> YosysOpenFheRunner::OpenFheOp_buffer(
    const std::vector<OpenFheBoolValue>& args) {
  XLS_CHECK_EQ(args.size(), 1);
  return args[0];
}

#define IMPL2(cell, GATE)                                                      \
  absl::StatusOr<OpenFheBoolValue> YosysOpenFheRunner::OpenFheOp_##cell(       \
      const std::vector<OpenFheBoolValue>& args) {                             \
    XLS_CHECK_EQ(args.size(), 2);                                              \
    return OpenFheBoolValue(                                                   \
        state_->cc_.EvalBinGate(lbcrypto::GATE, args[0].lwe(), args[1].lwe()), \
        state_->cc_);                                                          \
  }

IMPL2(and2, AND);
IMPL2(nand2, NAND);
IMPL2(or2, OR);
IMPL2(nor2, NOR);
IMPL2(xor2, XOR_FAST);
IMPL2(xnor2, XNOR_FAST);
#undef IMPL2

absl::Status YosysOpenFheRunner::Run(
    absl::Span<lbcrypto::LWECiphertext> result,
    std::vector<absl::Span<const lbcrypto::LWECiphertext>> in_args,
    std::vector<absl::Span<lbcrypto::LWECiphertext>> inout_args,
    lbcrypto::BinFHEContext cc) {
#define OP(name)                                              \
  {                                                           \
    #name, {                                                  \
      {                                                       \
        "Y",                                                  \
            [this](const std::vector<OpenFheBoolValue>& args) \
                -> absl::StatusOr<OpenFheBoolValue> {         \
              return this->OpenFheOp_##name(args);            \
            }                                                 \
      }                                                       \
    }                                                         \
  }
  if (state_ == nullptr) {
    xls::netlist::rtl::CellToOutputEvalFns<OpenFheBoolValue> openfhe_eval_map{
        OP(inv), OP(buffer), OP(and2), OP(nand2),
        OP(or2), OP(nor2),   OP(xor2), OP(xnor2),
    };

    XLS_RETURN_IF_ERROR(InitializeOnce(cc, openfhe_eval_map));
  }
#undef OP

  return state_->Run(result, in_args, inout_args);
}

absl::Status YosysOpenFheRunner::InitializeOnce(
    lbcrypto::BinFHEContext cc,
    const xls::netlist::rtl::CellToOutputEvalFns<OpenFheBoolValue>& eval_fns) {
  if (state_ == nullptr) {
    state_ = std::make_unique<YosysOpenFheRunnerState>(
        cc, *xls::netlist::cell_lib::CharStream::FromText(liberty_text_),
        xls::netlist::rtl::Scanner(netlist_text_));

    state_->netlist_ = std::move(
        *xls::netlist::rtl::AbstractParser<OpenFheBoolValue>::ParseNetlist(
            &state_->cell_library_, &state_->scanner_, state_->zero_,
            state_->one_));

    XLS_RETURN_IF_ERROR(state_->netlist_->AddCellEvaluationFns(eval_fns));

    XLS_CHECK(google::protobuf::TextFormat::ParseFromString(
        metadata_text_, &state_->metadata_));
  }
  return absl::OkStatus();
}

absl::Status YosysOpenFheRunner::YosysOpenFheRunnerState::Run(
    absl::Span<lbcrypto::LWECiphertext> result,
    std::vector<absl::Span<const lbcrypto::LWECiphertext>> in_args,
    std::vector<absl::Span<lbcrypto::LWECiphertext>> inout_args) {
  std::string function_name = metadata_.top_func_proto().name().name();
  XLS_ASSIGN_OR_RETURN(auto module, netlist_->GetModule(function_name));

  // Arguments are in the form of spans of LWECiphertexts, with one span per
  // input argument.  So for example, if you have two inputs, an uint8_t and an
  // int32_t, then you'll have two spans, the first of which is 8-bits wide, and
  // the second of which is 32-bits wide.  Each span entry will be a
  // LWECiphertext representing one bit of the input.  It represents a bit of
  // input, but it does not act like a "bool".
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
  //    YosysOpenFheRunner::OpenFheOp_xor2) to do the actual evaluation.  For
  //    the latter, we only need requirement (a) above, because all the actual
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

  using NetRef = xls::netlist::rtl::AbstractNetRef<OpenFheBoolValue>;
  std::vector<OpenFheBoolValue> input_bits;
  size_t in_i = 0, inout_i = 0;
  for (const auto& param : metadata_.top_func_proto().params()) {
    std::vector<OpenFheBoolValue> arg_bits;
    if (param.is_reference() && !param.is_const()) {
      XLS_CHECK(inout_i < inout_args.size());
      const auto& arg = inout_args[inout_i++];
      arg_bits.reserve(arg.size());
      for (int i = 0; i < arg.size(); i++) {
        arg_bits.emplace_back(arg[i], cc_);
      }
    } else {
      XLS_CHECK(in_i < in_args.size());
      const auto& arg = in_args[in_i++];
      arg_bits.reserve(arg.size());
      for (int i = 0; i < arg.size(); i++) {
        arg_bits.emplace_back(arg[i], cc_);
      }
    }
    input_bits.insert(input_bits.begin(), arg_bits.begin(), arg_bits.end());
  }
  std::reverse(input_bits.begin(), input_bits.end());

  xls::netlist::AbstractNetRef2Value<OpenFheBoolValue> input_nets;
  const std::vector<NetRef>& module_inputs = module->inputs();
  XLS_CHECK_EQ(module_inputs.size(), input_bits.size());

  for (int i = 0; i < module->inputs().size(); i++) {
    const NetRef in = module_inputs[i];
    XLS_CHECK(!input_nets.contains(in));
    input_nets.emplace(
        in, std::move(input_bits[module->GetInputPortOffset(in->name())]));
  }

  auto zero = OpenFheBoolValue::Unencrypted(false, cc_);
  auto one = OpenFheBoolValue::Unencrypted(true, cc_);
  // *2 for hyperthreading opportunities
  const int num_threads = sysconf(_SC_NPROCESSORS_ONLN) * 2;
  xls::netlist::AbstractInterpreter<OpenFheBoolValue> interpreter(
      netlist_.get(), zero, one, num_threads);
  XLS_ASSIGN_OR_RETURN(auto output_nets,
                       interpreter.InterpretModule(module, input_nets, {}));

  // The return value output_nets is a map from NetRef to OpenFheBoolValue
  // objects. Each of the OpenFheBoolValue objects contains an LWECiphertext,
  // which it either owns or has borrowed from elsewhere (whether it owns or has
  // borrowed does not matter here.)
  //
  // We need to map the output_nets-contained LWECiphertexts to the result. We
  // do that by assigning each pointer in the result array to the corresponding
  // pointer in the output_nets-owned LWECiphertexts.

  std::vector<lbcrypto::LWECiphertext> output_bit_vector;

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
  for (int i = 0; i < result.size(); i++, out++, copied++) {
    result[i] = *out;
  }

  XLS_CHECK(copied == output_bit_vector.size());
  XLS_CHECK(out == output_bit_vector.cend());

  return absl::OkStatus();
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
