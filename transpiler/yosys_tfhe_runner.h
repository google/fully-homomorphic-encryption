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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_TFHE_RUNNER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_TFHE_RUNNER_H_

#include <memory>

#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "google/protobuf/text_format.h"
#include "transpiler/data/tfhe_data.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/netlist/cell_library.h"
#include "xls/netlist/function_extractor.h"
#include "xls/netlist/interpreter.h"
#include "xls/netlist/lib_parser.h"
#include "xls/netlist/netlist_parser.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

class YosysTfheRunner;

// Class TfheBoolValue provides the glue that bridges the way the XLS netlist
// AbstractInterpreter processes boolean values, and the way the TFHE library
// handles them.  The former expects to see an object of a type that is
// source-code-compatible with bool, while the TFHE library uses opaque
// constructs of type LweSample that have no source-level compatibility with
// bool.
//
// Inputs to the top-level TFHE function are in the form of spans of LweSamples,
// with one span per input argument.  So for example, if you have two inputs, an
// uint8_t and an int32_t, then you'll have two spans, the first of which is
// 8-bits wide, and the second of which is 32-bits wide.  Each span entry will
// be a LweSample representing one bit of the input.  It represents a bit of
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
class TfheBoolValue {
 private:
  struct LweSingleSampleDeleter {
    void operator()(LweSample* lwe_sample) const {
      delete_gate_bootstrapping_ciphertext(lwe_sample);
    }
  };

 public:
  TfheBoolValue(bool val, const TFheGateBootstrappingCloudKeySet* bk)
      : bk_(bk) {
    std::unique_ptr<LweSample, LweSingleSampleDeleter> up(
        new_gate_bootstrapping_ciphertext(bk->params));
    lwe_ = std::move(up);
    bootsCONSTANT(lwe_.get(), val, bk);
  }

  TfheBoolValue(const TfheBoolValue& rhs) : lwe_(rhs.lwe_), bk_(rhs.bk_) {}
  TfheBoolValue& operator=(const TfheBoolValue& rhs) {
    lwe_ = rhs.lwe_;
    bk_ = rhs.bk_;
    return *this;
  }
  TfheBoolValue(TfheBoolValue&& rhs) {
    bk_ = rhs.bk_;
    lwe_ = std::move(rhs.lwe_);
  }
  TfheBoolValue& operator=(TfheBoolValue&& rhs) {
    bk_ = rhs.bk_;
    lwe_ = std::move(rhs.lwe_);
    return *this;
  }
  TfheBoolValue operator&(const TfheBoolValue& rhs) {
    std::unique_ptr<LweSample, LweSingleSampleDeleter> res(
        new_gate_bootstrapping_ciphertext(bk_->params));
    bootsAND(res.get(), lwe_.get(), rhs.lwe().get(), bk_);
    return TfheBoolValue(std::move(res), bk_);
  }
  TfheBoolValue operator|(const TfheBoolValue& rhs) {
    std::unique_ptr<LweSample, LweSingleSampleDeleter> res(
        new_gate_bootstrapping_ciphertext(bk_->params));
    bootsOR(res.get(), lwe_.get(), rhs.lwe().get(), bk_);
    return TfheBoolValue(std::move(res), bk_);
  }
  TfheBoolValue operator^(const TfheBoolValue& rhs) {
    std::unique_ptr<LweSample, LweSingleSampleDeleter> res(
        new_gate_bootstrapping_ciphertext(bk_->params));
    bootsXOR(res.get(), lwe_.get(), rhs.lwe().get(), bk_);
    return TfheBoolValue(std::move(res), bk_);
  }
  TfheBoolValue operator!() {
    std::unique_ptr<LweSample, LweSingleSampleDeleter> res(
        new_gate_bootstrapping_ciphertext(bk_->params));
    bootsNOT(res.get(), lwe_.get(), bk_);
    return TfheBoolValue(std::move(res), bk_);
  }
  const std::shared_ptr<LweSample> lwe() const { return lwe_; }

 private:
  std::shared_ptr<LweSample> lwe_;

 public:
  TfheBoolValue(const LweSample* lwe,
                const TFheGateBootstrappingCloudKeySet* bk)
      : lwe_(new_gate_bootstrapping_ciphertext(bk->params),
             LweSingleSampleDeleter()),
        bk_(bk) {
    bootsCOPY(lwe_.get(), lwe, bk_);
  }
  TfheBoolValue(std::unique_ptr<LweSample, LweSingleSampleDeleter> lwe,
                const TFheGateBootstrappingCloudKeySet* bk)
      : bk_(bk) {
    lwe_ = std::move(lwe);
  }

 private:
  const TFheGateBootstrappingCloudKeySet* bk_;
};

class YosysTfheRunner {
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
  YosysTfheRunner(const std::string& liberty_text,
                  const std::string& netlist_text,
                  const std::string& metadata_text)
      : liberty_text_(liberty_text),
        netlist_text_(netlist_text),
        metadata_text_(metadata_text),
        state_(nullptr) {}

  absl::Status InitializeOnce(
      const TFheGateBootstrappingCloudKeySet* bk,
      const xls::netlist::rtl::CellToOutputEvalFns<TfheBoolValue>& eval_fns);

  absl::Status Run(absl::Span<LweSample> result,
                   std::vector<absl::Span<const LweSample>> in_args,
                   std::vector<absl::Span<LweSample>> inout_args,
                   const TFheGateBootstrappingCloudKeySet* bk);

  std::unique_ptr<TfheBoolValue> CreateTfheBoolValue(bool in) {
    XLS_CHECK(state_->bk_ != nullptr);
    return std::make_unique<TfheBoolValue>(in, state_->bk_);
  }

 private:
  struct YosysTfheRunnerState {
    YosysTfheRunnerState(const TFheGateBootstrappingCloudKeySet* bk,
                         xls::netlist::cell_lib::CharStream char_stream,
                         xls::netlist::rtl::Scanner scanner)
        : bk_(bk),
          zero_(false, bk_),
          one_(true, bk_),
          char_stream_(std::move(char_stream)),
          lib_proto_(*xls::netlist::function::ExtractFunctions(&char_stream_)),
          cell_library_(
              *xls::netlist::AbstractCellLibrary<TfheBoolValue>::FromProto(
                  lib_proto_, zero_, one_)),
          scanner_(scanner) {}

    absl::Status Run(absl::Span<LweSample> result,
                     std::vector<absl::Span<const LweSample>> in_args,
                     std::vector<absl::Span<LweSample>> inout_args);

    const TFheGateBootstrappingCloudKeySet* bk_;
    TfheBoolValue zero_;
    TfheBoolValue one_;
    xls::netlist::cell_lib::CharStream char_stream_;
    xls::netlist::CellLibraryProto lib_proto_;
    xls::netlist::AbstractCellLibrary<TfheBoolValue> cell_library_;
    xls::netlist::rtl::Scanner scanner_;
    std::unique_ptr<xls::netlist::rtl::AbstractNetlist<TfheBoolValue>> netlist_;
    xlscc_metadata::MetadataOutput metadata_;
  };

  absl::StatusOr<TfheBoolValue> TfheOp_inv(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_buffer(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_and2(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_nand2(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_or2(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_andyn2(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_andny2(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_oryn2(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_orny2(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_nor2(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_xor2(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_xnor2(
      const std::vector<TfheBoolValue>& args);
  absl::StatusOr<TfheBoolValue> TfheOp_imux2(
      const std::vector<TfheBoolValue>& args);

  const std::string liberty_text_;
  const std::string netlist_text_;
  const std::string metadata_text_;
  std::unique_ptr<YosysTfheRunnerState> state_;
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_TFHE_RUNNER_H_
