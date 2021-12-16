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
#include "google/protobuf/text_format.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/ir/bits.h"
#include "xls/ir/bits_ops.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

using NetRef = xls::netlist::rtl::NetRef;
using NetlistParser = xls::netlist::rtl::Parser;

absl::Status YosysRunner::CreateAndRun(const std::string& netlist_text,
                                       const std::string& metadata_text,
                                       const std::string& liberty_text,
                                       absl::Span<bool> result,
                                       std::vector<absl::Span<bool>> args) {
  XLS_ASSIGN_OR_RETURN(
      auto char_stream,
      xls::netlist::cell_lib::CharStream::FromText(liberty_text));
  XLS_ASSIGN_OR_RETURN(auto lib_proto,
                       xls::netlist::function::ExtractFunctions(&char_stream));
  XLS_ASSIGN_OR_RETURN(auto cell_library, xls::netlist::CellLibrary::FromProto(
                                              lib_proto, false, true));
  xls::netlist::rtl::Scanner scanner(netlist_text);
  XLS_ASSIGN_OR_RETURN(auto netlist, NetlistParser::ParseNetlist(
                                         &cell_library, &scanner, false, true));

  xlscc_metadata::MetadataOutput metadata;
  if (!google::protobuf::TextFormat::ParseFromString(metadata_text,
                                                     &metadata)) {
    return absl::InvalidArgumentError(
        "Could not parse function metadata proto.");
  }

  std::string function_name = metadata.top_func_proto().name().name();
  XLS_ASSIGN_OR_RETURN(auto module, netlist->GetModule(function_name));

  xls::Bits input_bits;
  for (auto arg : args) {
    xls::Bits arg_bits(arg);
    input_bits = xls::bits_ops::Concat({input_bits, arg_bits});
  }
  input_bits = xls::bits_ops::Reverse(input_bits);

  absl::flat_hash_map<const NetRef, bool> input_nets;
  const std::vector<NetRef>& module_inputs = module->inputs();
  XLS_CHECK(module_inputs.size() == input_bits.bit_count());

  for (int i = 0; i < module->inputs().size(); i++) {
    const NetRef in = module_inputs[i];
    XLS_CHECK(!input_nets.contains(in));
    input_nets[in] = input_bits.Get(module->GetInputPortOffset(in->name()));
    //    std::cout << "in->name(): " << in->name() << std::endl;
  }

  xls::netlist::Interpreter interpreter(netlist.get(), false, true);
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

  // The return value of the function will always come first, so we copy that.
  // If there is no return value, then result.size() == 0 and we do not copy
  // anything.
  std::copy_n(out, result.size(), result.begin());

  out += result.size();

  // The remaining output wires in the netlist follow the declaration order of
  // the input wires in the verilog file.  Support you have the following
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
  for (int i = 0; i < module_inputs.size();) {
    // Start by pulling off the first input net wire.  Following the example
    // above, it will have the name "c[0]".  (When we get to the single-wire "a"
    // next, the name will simply be "a".).
    std::vector<std::string> name_and_idx =
        absl::StrSplit(module_inputs[i]->name(), '[');
    // Look for the non-indexed name ("c") in the example above in the list of
    // function arguments, which we can access from the metadata.
    auto found = std::find_if(
        metadata.top_func_proto().params().cbegin(),
        metadata.top_func_proto().params().cend(),
        [&name_and_idx](const xlscc_metadata::FunctionParameter& arg) {
          return arg.name() == name_and_idx[0];
        });
    // We must be able to find that parameter--failing to is a bug, as we
    // autogenerate both the netlist and the metadata from the same source file,
    // and provide these parameters to this method.
    XLS_CHECK(found != metadata.top_func_proto().params().cend());
    // Find the index of the argument for our match.  In our example, it will be
    // 2, since args[2] is the span for the encoded form of argument "c".
    size_t params_i =
        std::distance(metadata.top_func_proto().params().begin(), found);
    // Get the bit size of the argument (e.g., 8 since "c" is defined to be a
    // byte.)
    auto arg_size = args[params_i].size();

    if (found->is_reference() && !found->is_const()) {
      // Now, the out[i] is the return value for c[0], out[i+1] the return value
      // for c[1], and so on.  Copy that range directly into the span
      // representing argument "c", which is exactly args[2].
      //
      // NOTE: We make a big assumption here, which is that the remaining wires
      // will follow the order 0, 1, 2, ... and so on rather than some random
      // order.  If that assumption does not hold, then we can replace this copy
      // with a single bit and run this loop for each input wire.
      std::copy_n(out + i, arg_size, args[params_i].begin());
      // Consistency check: if we saw an indexed name, e.g. "c[?]", then assert
      // that ? == 0.  This does not fully enforce the assumption we make above,
      // but helps a bit.
      if (name_and_idx.size() == 2) {
        absl::string_view idx = absl::StripSuffix(name_and_idx[1], "]");
        int64_t idx_out;
        XLS_CHECK(absl::SimpleAtoi(idx, &idx_out));
        XLS_CHECK(idx_out == 0);
      }
      copied += arg_size;
    }

    // Now that we've handled (either copied or skipped) the result into the
    // argument, skip ahead by the bit size of the argument.
    i += arg_size;
  }
  XLS_CHECK(copied == output_bit_vector.size());

  return absl::OkStatus();
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
