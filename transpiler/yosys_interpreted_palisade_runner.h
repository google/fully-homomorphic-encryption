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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_RUNNER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_RUNNER_H_

#include <memory>

#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "google/protobuf/text_format.h"
#include "palisade/binfhe/binfhecontext.h"
#include "palisade/binfhe/ringcore.h"
#include "transpiler/data/palisade_data.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/netlist/cell_library.h"
#include "xls/netlist/function_extractor.h"
#include "xls/netlist/interpreter.h"
#include "xls/netlist/lib_parser.h"
#include "xls/netlist/netlist_parser.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

class YosysTfheRunner;

// Class PalisadeBoolValue provides the glue that bridges the way the XLS
// netlist AbstractInterpreter processes boolean values, and the way the
// PALISADE BinFHE library handles them.  The former expects to see an object of
// a type that is source-code-compatible with bool, while the BinFHE library
// uses opaque constructs of type LweCiphertext that have no source-level
// compatibility with bool.
//
// Inputs to the top-level TFHE function are in the form of spans of
// LweCiphertext, with one span per input argument.  So for example, if you have
// two inputs, an uint8_t and an int32_t, then you'll have two spans, the first
// of which is 8-bits wide, and the second of which is 32-bits wide.  Each span
// entry will be a LweCiphertext representing one bit of the input.  It
// represents a bit of input, but it does not act like a "bool".
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
//    YosysTfheRunner::PalisadeOp_xor2) to do the actual evaluation.  For the
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
class PalisadeBoolValue {
 public:
  static PalisadeBoolValue Unencrypted(bool val, lbcrypto::BinFHEContext cc) {
    return PalisadeBoolValue(cc.EvalConstant(val), cc);
  }

  PalisadeBoolValue(const PalisadeBoolValue& rhs)
      : lwe_(std::make_shared<lbcrypto::LWECiphertextImpl>(*rhs.lwe_)),
        cc_(rhs.cc_) {}
  PalisadeBoolValue& operator=(const PalisadeBoolValue& rhs) {
    lwe_ = std::make_shared<lbcrypto::LWECiphertextImpl>(*rhs.lwe_);
    cc_ = rhs.cc_;
    return *this;
  }
  PalisadeBoolValue(PalisadeBoolValue&& rhs) {
    cc_ = rhs.cc_;
    lwe_ = std::move(rhs.lwe_);
  }
  PalisadeBoolValue& operator=(PalisadeBoolValue&& rhs) {
    cc_ = rhs.cc_;
    lwe_ = std::move(rhs.lwe_);
    return *this;
  }
  PalisadeBoolValue operator&(const PalisadeBoolValue& rhs) {
    return PalisadeBoolValue(cc_.EvalBinGate(lbcrypto::AND, lwe_, rhs.lwe()),
                             cc_);
  }
  PalisadeBoolValue operator|(const PalisadeBoolValue& rhs) {
    return PalisadeBoolValue(cc_.EvalBinGate(lbcrypto::OR, lwe_, rhs.lwe()),
                             cc_);
  }
  PalisadeBoolValue operator^(const PalisadeBoolValue& rhs) {
    return PalisadeBoolValue(cc_.EvalBinGate(lbcrypto::XOR, lwe_, rhs.lwe()),
                             cc_);
  }
  PalisadeBoolValue operator!() {
    return PalisadeBoolValue(cc_.EvalNOT(lwe_), cc_);
  }
  lbcrypto::LWECiphertext lwe() const { return lwe_; }

 private:
  lbcrypto::LWECiphertext lwe_;

 public:
  PalisadeBoolValue(lbcrypto::LWECiphertext lwe, lbcrypto::BinFHEContext cc)
      : lwe_(lwe), cc_(cc) {}

 private:
  lbcrypto::BinFHEContext cc_;
};

class YosysPalisadeRunner {
 public:
  // char_stream: value
  //   --> lib_proto: value
  //      --> cell_library: value
  //
  //  netlist_text
  //    --> scanner: value
  //
  //  &cell_library, &scanner
  //    --> netlist: pointer
  YosysPalisadeRunner(const std::string& liberty_text,
                      const std::string& netlist_text,
                      const std::string& metadata_text)
      : liberty_text_(liberty_text),
        netlist_text_(netlist_text),
        metadata_text_(metadata_text),
        state_(nullptr) {}

  absl::Status InitializeOnce(
      lbcrypto::BinFHEContext cc,
      const xls::netlist::rtl::CellToOutputEvalFns<PalisadeBoolValue>&
          eval_fns);

  absl::Status Run(
      absl::Span<lbcrypto::LWECiphertext> result,
      std::vector<absl::Span<const lbcrypto::LWECiphertext>> in_args,
      std::vector<absl::Span<lbcrypto::LWECiphertext>> inout_args,
      lbcrypto::BinFHEContext cc);

  std::unique_ptr<PalisadeBoolValue> CreatePalisadeBoolValue(bool in) {
    return std::make_unique<PalisadeBoolValue>(
        PalisadeBoolValue::Unencrypted(in, state_->cc_));
  }

 private:
  struct YosysPalisadeRunnerState {
    YosysPalisadeRunnerState(lbcrypto::BinFHEContext cc,
                             xls::netlist::cell_lib::CharStream char_stream,
                             xls::netlist::rtl::Scanner scanner)
        : cc_(cc),
          zero_(PalisadeBoolValue::Unencrypted(false, cc_)),
          one_(PalisadeBoolValue::Unencrypted(true, cc_)),
          char_stream_(std::move(char_stream)),
          lib_proto_(*xls::netlist::function::ExtractFunctions(&char_stream_)),
          cell_library_(
              *xls::netlist::AbstractCellLibrary<PalisadeBoolValue>::FromProto(
                  lib_proto_, zero_, one_)),
          scanner_(scanner) {}

    absl::Status Run(
        absl::Span<lbcrypto::LWECiphertext> result,
        std::vector<absl::Span<const lbcrypto::LWECiphertext>> in_args,
        std::vector<absl::Span<lbcrypto::LWECiphertext>> inout_args);

    lbcrypto::BinFHEContext cc_;
    PalisadeBoolValue zero_;
    PalisadeBoolValue one_;
    xls::netlist::cell_lib::CharStream char_stream_;
    xls::netlist::CellLibraryProto lib_proto_;
    xls::netlist::AbstractCellLibrary<PalisadeBoolValue> cell_library_;
    xls::netlist::rtl::Scanner scanner_;
    std::unique_ptr<xls::netlist::rtl::AbstractNetlist<PalisadeBoolValue>>
        netlist_;
    xlscc_metadata::MetadataOutput metadata_;
  };

  absl::StatusOr<PalisadeBoolValue> PalisadeOp_inv(
      const std::vector<PalisadeBoolValue>& args);
  absl::StatusOr<PalisadeBoolValue> PalisadeOp_buffer(
      const std::vector<PalisadeBoolValue>& args);
  absl::StatusOr<PalisadeBoolValue> PalisadeOp_and2(
      const std::vector<PalisadeBoolValue>& args);
  absl::StatusOr<PalisadeBoolValue> PalisadeOp_nand2(
      const std::vector<PalisadeBoolValue>& args);
  absl::StatusOr<PalisadeBoolValue> PalisadeOp_or2(
      const std::vector<PalisadeBoolValue>& args);
  absl::StatusOr<PalisadeBoolValue> PalisadeOp_nor2(
      const std::vector<PalisadeBoolValue>& args);
  absl::StatusOr<PalisadeBoolValue> PalisadeOp_xor2(
      const std::vector<PalisadeBoolValue>& args);
  absl::StatusOr<PalisadeBoolValue> PalisadeOp_xnor2(
      const std::vector<PalisadeBoolValue>& args);

  const std::string liberty_text_;
  const std::string netlist_text_;
  const std::string metadata_text_;
  std::unique_ptr<YosysPalisadeRunnerState> state_;
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_RUNNER_H_
