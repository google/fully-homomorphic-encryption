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

#include <vector>

#include "absl/status/statusor.h"
#include "absl/types/optional.h"
#include "palisade/binfhe/binfhecontext.h"
#include "transpiler/abstract_xls_runner.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

class PalisadeRunner
    : public AbstractXlsRunner<PalisadeRunner, lbcrypto::LWECiphertext> {
 private:
  using Base = AbstractXlsRunner<PalisadeRunner, lbcrypto::LWECiphertext>;

  class PalisadeOperations : public BitOperations {
   public:
    PalisadeOperations(lbcrypto::BinFHEContext cc) : cc_(cc) {}
    virtual ~PalisadeOperations() {}

    lbcrypto::LWECiphertext And(const lbcrypto::LWECiphertext lhs,
                                const lbcrypto::LWECiphertext rhs) override;
    lbcrypto::LWECiphertext Or(const lbcrypto::LWECiphertext lhs,
                               const lbcrypto::LWECiphertext rhs) override;
    lbcrypto::LWECiphertext Not(const lbcrypto::LWECiphertext in) override;

    lbcrypto::LWECiphertext Constant(bool value) override;

    void Copy(const lbcrypto::LWECiphertext src,
              lbcrypto::LWECiphertext dst) override;

    lbcrypto::LWECiphertext CopyOf(const lbcrypto::LWECiphertext src) override;

   private:
    lbcrypto::BinFHEContext cc_;
  };

 public:
  using Base::AbstractXlsRunner;
  using Base::CreateFromFile;
  using Base::CreateFromStrings;

  absl::Status Run(
      absl::Span<lbcrypto::LWECiphertext> result,
      absl::flat_hash_map<std::string, absl::Span<lbcrypto::LWECiphertext>>
          args,
      lbcrypto::BinFHEContext cc);
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_PALISADE_RUNNER_H_
