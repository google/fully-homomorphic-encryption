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

// Helper functions and fixtures for TFHE tests.

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TESTS_TFHE_TEST_UTIL_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TESTS_TFHE_TEST_UTIL_H_

#include <array>

#include "absl/memory/memory.h"
#include "gtest/gtest.h"
#include "transpiler/data/tfhe_data.h"

constexpr int kMainMinimumLambda = 120;

// Random seed for key generation
// Note: In real applications, a cryptographically secure seed needs to be used.
constexpr std::array<uint32_t, 3> kSeed = {314, 1592, 657};

namespace fully_homomorphic_encryption {
namespace transpiler {

class TranspilerTestBase : public ::testing::Test {
 public:
  const TFheGateBootstrappingParameterSet* params() const {
    return params_->get();
  }
  const TFheGateBootstrappingSecretKeySet* secret_key() const {
    return key_->get();
  }
  const TFheGateBootstrappingCloudKeySet* cloud_key() const {
    return key_->cloud();
  }

 protected:
  static void SetUpTestSuite() {
    params_ = new TFHEParameters(kMainMinimumLambda);
    key_ = new TFHESecretKeySet(*params_, kSeed);
  }

  static void TearDownTestSuite() {
    delete params_;
    params_ = nullptr;

    delete key_;
    key_ = nullptr;
  }

  ~TranspilerTestBase() override {}

 private:
  static TFHEParameters* params_;
  static TFHESecretKeySet* key_;
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TESTS_TFHE_TEST_UTIL_H_
