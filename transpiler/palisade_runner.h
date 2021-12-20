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

// Funcionality for "interpreting" XLS IR within a TFHE environment.
//
// IR must satisfy:
//
// * Every data type is bits.
// * Only params and return values have width > 1.
// * The return value is a CONCAT node.

// Usage:
//
// auto runner = PalisadeRunner::Create("/path/to/ir", "my_package");
// auto bk = ... set up keys ..
// auto result == ... set up LweSample* with the right width ...
// auto args = absl::flat_hash_map {{"x", ...}, {"y", ...}};
// runner.Run(result, args, bk);

// See tfhe_runner_test.cc.

#ifndef THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_PALISADE_RUNNER_H_
#define THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_PALISADE_RUNNER_H_

#include <pthread.h>
#include <semaphore.h>

#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "palisade/binfhe/binfhecontext.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/ir/function.h"
#include "xls/ir/package.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

class PalisadeRunner {
 public:
  PalisadeRunner(std::unique_ptr<xls::Package> package,
                 xlscc_metadata::MetadataOutput metadata);
  ~PalisadeRunner();

  absl::Status Run(
      absl::Span<lbcrypto::LWECiphertext> result,
      absl::flat_hash_map<std::string, absl::Span<lbcrypto::LWECiphertext>>
          args,
      lbcrypto::BinFHEContext cc);

  static absl::StatusOr<std::unique_ptr<PalisadeRunner>> CreateFromFile(
      absl::string_view ir_path, absl::string_view metadata_path);

  static absl::StatusOr<std::unique_ptr<PalisadeRunner>> CreateFromStrings(
      absl::string_view xls_package, absl::string_view metadata_text);

 private:
  absl::StatusOr<xls::Function*> GetEntry() {
    return package_->GetFunction(metadata_.top_func_proto().name().name());
  }

  // This method copies the relevant bit from input params into the result.
  static absl::Status HandleBitSlice(
      lbcrypto::LWECiphertext& result, const xls::BitSlice* bit_slice,
      absl::flat_hash_map<std::string, absl::Span<lbcrypto::LWECiphertext>>
          args,
      lbcrypto::BinFHEContext cc);

  // Array support will need to be updated when structs are added: it could be
  // possible that there is padding present between subsequent elements in an
  // array of these structs that is not captured by the corresponding XLS type -
  // for example, a 56-byte struct will likely be padded out to 64 bytes
  // internally. This code would assume that struct data is all packed, and thus
  // the output would be garbled. Host layout will need to be considered here.
  absl::Status CollectNodeValue(
      const xls::Node* node, absl::Span<lbcrypto::LWECiphertext> output_arg,
      int output_offset,
      const absl::flat_hash_map</*id=*/uint64_t, lbcrypto::LWECiphertext>&
          values,
      lbcrypto::BinFHEContext cc);

  // Walks the type elements comprising `function`'s output type and generates
  // FHE copy operations to extract the data corresponding to each.
  //
  // At present, `function`'s output must be of the form (A, B), where A is
  // bits- or array-typed, and B must be a tuple containing only bits- or
  // array-typed elements. A corresponds to the output from the original C++
  // function itself, and the elements of B are the in/out params to the
  // function. We don't currently have the ability to traverse the definition of
  // any given [C/C++] struct, so struct/tuple types are not _currently_
  // supported, though this is intended to change in the near future.
  absl::Status CollectOutputs(
      absl::Span<lbcrypto::LWECiphertext> result,
      absl::flat_hash_map<std::string, absl::Span<lbcrypto::LWECiphertext>>
          args,
      const absl::flat_hash_map</*id=*/uint64_t, lbcrypto::LWECiphertext>&
          values,
      lbcrypto::BinFHEContext cc);

  static void* ThreadBodyStatic(void* runner);
  absl::Status ThreadBody();

  // This is static to ensure no access to lock-protected state
  // Can return nullptr for no-ops
  static absl::StatusOr<lbcrypto::LWECiphertext> EvalSingleOp(
      xls::Node* n, std::vector<lbcrypto::LWECiphertext> operands,
      const absl::flat_hash_map<std::string,
                                absl::Span<lbcrypto::LWECiphertext>>
          args,
      lbcrypto::BinFHEContext cc);

  absl::flat_hash_map<std::string, absl::Span<lbcrypto::LWECiphertext>>
      const_args_;
  lbcrypto::BinFHEContext const_cc_;

  typedef std::tuple<xls::Node*, std::vector<lbcrypto::LWECiphertext>>
      NodeToEval;

  pthread_mutex_t lock_;  // Only used by worker threads

  sem_t input_sem_;
  std::queue<NodeToEval> input_queue_;  // Protected by lock_ in worker threads

  typedef std::tuple<xls::Node*, lbcrypto::LWECiphertext> NodeFromEval;
  sem_t output_sem_;
  std::queue<NodeFromEval>
      output_queue_;  // Protected by lock_ in worker threads

  std::atomic<bool> threads_should_exit_;

  std::unique_ptr<xls::Package> package_;
  std::string function_name_;
  std::vector<pthread_t> threads_;
  xlscc_metadata::MetadataOutput metadata_;
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_PALISADE_RUNNER_H_
