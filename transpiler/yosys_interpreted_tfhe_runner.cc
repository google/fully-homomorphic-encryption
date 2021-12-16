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

#include "transpiler/yosys_interpreted_tfhe_runner.h"

#include "absl/container/flat_hash_map.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/text_format.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/ir/bits.h"
#include "xls/ir/bits_ops.h"
#include "xls/netlist/netlist.h"

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

absl::Status YosysTfheRunner::Run(absl::Span<LweSample> result,
                                  std::vector<absl::Span<LweSample>> args,
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

  return state_->Run(result, args);
}

absl::Status YosysTfheRunner::InitializeOnce(
    const TFheGateBootstrappingCloudKeySet* bk,
    const xls::netlist::rtl::CellToOutputEvalFns<TfheBoolValue>& eval_fns) {
  if (state_ == nullptr) {
    state_ = std::make_unique<YosysTfheRunnerState>(
        bk, *xls::netlist::cell_lib::CharStream::FromText(liberty_text_),
        xls::netlist::rtl::Scanner(netlist_text_));

    std::cout << "Parsing netlist..." << std::endl;
    auto start_time = absl::Now();
    state_->netlist_ = std::move(
        *xls::netlist::rtl::AbstractParser<TfheBoolValue>::ParseNetlist(
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

absl::Status YosysTfheRunner::YosysTfheRunnerState::Run(
    absl::Span<LweSample> result, std::vector<absl::Span<LweSample>> args) {
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

  std::cout << "Setting up inputs." << std::endl;
  using NetRef = xls::netlist::rtl::AbstractNetRef<TfheBoolValue>;
  std::vector<TfheBoolValue> input_bits;
  for (auto arg : args) {
    std::vector<TfheBoolValue> arg_bits;
    arg_bits.reserve(arg.size());
    for (int i = 0; i < arg.size(); i++) {
      arg_bits.emplace_back(arg.data() + i, bk_);
    }
    input_bits.insert(input_bits.begin(), arg_bits.begin(), arg_bits.end());
  }
  std::reverse(input_bits.begin(), input_bits.end());

  absl::flat_hash_map<const NetRef, TfheBoolValue> input_nets;
  const std::vector<NetRef>& module_inputs = module->inputs();
  XLS_CHECK(module_inputs.size() == input_bits.size());

  for (int i = 0; i < module->inputs().size(); i++) {
    const NetRef in = module_inputs[i];
    XLS_CHECK(!input_nets.contains(in));
    input_nets.emplace(
        in, std::move(input_bits[module->GetInputPortOffset(in->name())]));
    //    std::cout << "in->name(): " << in->name() << std::endl;
  }
  std::cout << "Done setting up inputs." << std::endl;

  std::cout << "Interpreting." << std::endl;
  TfheBoolValue zero(false, bk_);
  TfheBoolValue one(true, bk_);
  xls::netlist::AbstractInterpreter<TfheBoolValue> interpreter(netlist_.get(),
                                                               zero, one);
  XLS_ASSIGN_OR_RETURN(auto output_nets,
                       interpreter.InterpretModule(module, input_nets, {}));

  std::cout << "Collecting outputs." << std::endl;

  // The return value output_nets is a map from NetRef to FheBoolValue objects.
  // Each of the FheBoolValue objects contains a LweSample*, which it either
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

  // The return value of the function will always come first, so we copy that.
  // If there is no return value, then result.size() == 0 and we do not copy
  // anything.
  for (int i = 0; i < result.size(); i++) {
    LweSample* lwe = result.data();
    bootsCOPY(&lwe[i], out->get(), bk_);
    out++;
  }

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
      const LweSample* src = (out + i)->get();
      LweSample* dest = args[params_i].data();
      bootsCOPY(&dest[params_i_idx], src, bk_);
      copied++;
    }
  }
  XLS_CHECK(copied == output_bit_vector.size());

  std::cout << "Done." << std::endl;

  return absl::OkStatus();
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
