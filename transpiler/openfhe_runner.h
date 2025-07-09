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

// Funcionality for "interpreting" XLS IR within a OpenFHE environment.
//
// IR must satisfy:
//
// * Every data type is bits.
// * Only params and return values have width > 1.
// * The return value is a CONCAT node.

// Usage:
//
// auto runner = OpenFheRunner::Create("/path/to/ir", "my_package");
// auto bk = ... set up keys ..
// auto result == ... set up LweSample* with the right width ...
// auto args = absl::flat_hash_map {{"x", ...}, {"y", ...}};
// runner.Run(result, args, bk);

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_OPENFHE_RUNNER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_OPENFHE_RUNNER_H_

#include <vector>

#include "absl/status/statusor.h"
#include "absl/types/optional.h"
#include "src/binfhe/include/binfhecontext.h"
#include "transpiler/abstract_xls_runner.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

using OpenFheCiphertext = lbcrypto::LWECiphertext;
using OpenFheCiphertextRef = OpenFheCiphertext;
using OpenFheCiphertextConstRef = const OpenFheCiphertextRef;

class OpenFheRunner : public AbstractXlsRunner<OpenFheRunner, OpenFheCiphertext,
                                               OpenFheCiphertextRef,
                                               OpenFheCiphertextConstRef> {
 private:
  using Base =
      AbstractXlsRunner<OpenFheRunner, OpenFheCiphertext, OpenFheCiphertextRef,
                        OpenFheCiphertextConstRef>;

  class OpenFheOperations : public BitOperations {
   public:
    OpenFheOperations(lbcrypto::BinFHEContext cc) : cc_(cc) {}
    virtual ~OpenFheOperations() {}

    OpenFheCiphertext And(OpenFheCiphertextConstRef lhs,
                          OpenFheCiphertextConstRef rhs) override;
    OpenFheCiphertext Or(OpenFheCiphertextConstRef lhs,
                         OpenFheCiphertextConstRef rhs) override;
    OpenFheCiphertext Not(OpenFheCiphertextConstRef in) override;

    OpenFheCiphertext Constant(bool value) override;

    void Copy(OpenFheCiphertextConstRef src,
              OpenFheCiphertextRef& dst) override;

    OpenFheCiphertext CopyOf(OpenFheCiphertextConstRef src) override;

   private:
    lbcrypto::BinFHEContext cc_;
  };

 public:
  using Base::AbstractXlsRunner;
  using Base::CreateFromFile;
  using Base::CreateFromStrings;

  absl::Status Run(
      absl::Span<OpenFheCiphertextRef> result,
      absl::flat_hash_map<std::string, absl::Span<OpenFheCiphertextConstRef>>
          in_args,
      absl::flat_hash_map<std::string, absl::Span<OpenFheCiphertextRef>>
          inout_args,
      lbcrypto::BinFHEContext cc);
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_OPENFHE_RUNNER_H_
