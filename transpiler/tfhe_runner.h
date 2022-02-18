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
// auto runner = TfheRunner::Create("/path/to/ir", "my_package");
// auto bk = ... set up keys ..
// auto result == ... set up LweSample* with the right width ...
// auto args = absl::flat_hash_map {{"x", ...}, {"y", ...}};
// runner.Run(result, args, bk);

// See tfhe_runner_test.cc.

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TFHE_RUNNER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TFHE_RUNNER_H_

#include <vector>

#include "absl/status/statusor.h"
#include "absl/types/optional.h"
#include "tfhe/tfhe.h"
#include "transpiler/abstract_xls_runner.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

class TfheCiphertextRef {
 public:
  TfheCiphertextRef(LweSample* value) : value_(value) {}

  LweSample* get() { return value_; }
  const LweSample* get() const { return value_; }

 private:
  LweSample* value_;
};

class TfheCiphertextConstRef {
 public:
  TfheCiphertextConstRef(const LweSample* value) : value_(value) {}
  TfheCiphertextConstRef(const TfheCiphertextRef& ref) : value_(ref.get()) {}
  const LweSample* get() const { return value_; }

 private:
  const LweSample* value_;
};

class TfheCiphertext {
 public:
  TfheCiphertext(const TFheGateBootstrappingParameterSet* params)
      : value_(new_gate_bootstrapping_ciphertext(params)) {}

  LweSample* get() { return value_.get(); }
  const LweSample* get() const { return value_.get(); }

  operator TfheCiphertextRef() { return TfheCiphertextRef(value_.get()); }
  operator TfheCiphertextConstRef() const {
    return TfheCiphertextConstRef(value_.get());
  }

 private:
  struct LweSampleSingletonDeleter {
    void operator()(LweSample* lwe_sample) const {
      delete_gate_bootstrapping_ciphertext(lwe_sample);
    }
  };

  std::unique_ptr<LweSample, LweSampleSingletonDeleter> value_;
};

class TfheRunner
    : public AbstractXlsRunner<TfheRunner, TfheCiphertext, TfheCiphertextRef,
                               TfheCiphertextConstRef> {
 private:
  using Base = AbstractXlsRunner<TfheRunner, TfheCiphertext, TfheCiphertextRef,
                                 TfheCiphertextConstRef>;

  class TfheOperations : public BitOperations {
   public:
    TfheOperations(const TFheGateBootstrappingCloudKeySet* bk) : bk_(bk) {}
    virtual ~TfheOperations() {}

    TfheCiphertext And(TfheCiphertextConstRef lhs,
                       TfheCiphertextConstRef rhs) override;
    TfheCiphertext Or(TfheCiphertextConstRef lhs,
                      TfheCiphertextConstRef rhs) override;
    TfheCiphertext Not(TfheCiphertextConstRef in) override;

    TfheCiphertext Constant(bool value) override;

    void Copy(TfheCiphertextConstRef src, TfheCiphertextRef& dst) override;

    TfheCiphertext CopyOf(TfheCiphertextConstRef src) override;

   private:
    const TFheGateBootstrappingCloudKeySet* bk_;
  };

 public:
  using Base::AbstractXlsRunner;
  using Base::CreateFromFile;
  using Base::CreateFromStrings;

  absl::Status Run(
      absl::Span<LweSample> result,
      absl::flat_hash_map<std::string, absl::Span<const LweSample>> in_args,
      absl::flat_hash_map<std::string, absl::Span<LweSample>> inout_args,
      const TFheGateBootstrappingCloudKeySet* bk);
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TFHE_RUNNER_H_
