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

// Funcionality for "interpreting" XLS IR within a PALISADE environment.
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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_PALISADE_RUNNER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_PALISADE_RUNNER_H_

#include <vector>

#include "absl/status/statusor.h"
#include "absl/types/optional.h"
#include "palisade/binfhe/binfhecontext.h"
#include "transpiler/abstract_xls_runner.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

using PalisadeCiphertext = lbcrypto::LWECiphertext;
using PalisadeCiphertextRef = PalisadeCiphertext;
using PalisadeCiphertextConstRef = const PalisadeCiphertextRef;

class PalisadeRunner
    : public AbstractXlsRunner<PalisadeRunner, PalisadeCiphertext,
                               PalisadeCiphertextRef,
                               PalisadeCiphertextConstRef> {
 private:
  using Base =
      AbstractXlsRunner<PalisadeRunner, PalisadeCiphertext,
                        PalisadeCiphertextRef, PalisadeCiphertextConstRef>;

  class PalisadeOperations : public BitOperations {
   public:
    PalisadeOperations(lbcrypto::BinFHEContext cc) : cc_(cc) {}
    virtual ~PalisadeOperations() {}

    PalisadeCiphertext And(PalisadeCiphertextConstRef lhs,
                           PalisadeCiphertextConstRef rhs) override;
    PalisadeCiphertext Or(PalisadeCiphertextConstRef lhs,
                          PalisadeCiphertextConstRef rhs) override;
    PalisadeCiphertext Not(PalisadeCiphertextConstRef in) override;

    PalisadeCiphertext Constant(bool value) override;

    void Copy(PalisadeCiphertextConstRef src,
              PalisadeCiphertextRef& dst) override;

    PalisadeCiphertext CopyOf(PalisadeCiphertextConstRef src) override;

   private:
    lbcrypto::BinFHEContext cc_;
  };

 public:
  using Base::AbstractXlsRunner;
  using Base::CreateFromFile;
  using Base::CreateFromStrings;

  absl::Status Run(
      absl::Span<PalisadeCiphertextRef> result,
      absl::flat_hash_map<std::string, absl::Span<PalisadeCiphertextConstRef>>
          in_args,
      absl::flat_hash_map<std::string, absl::Span<PalisadeCiphertextRef>>
          inout_args,
      lbcrypto::BinFHEContext cc);
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_PALISADE_RUNNER_H_
