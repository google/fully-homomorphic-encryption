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

// Helper functions and fixtures for OpenFHE tests.

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TESTS_OPENFHE_TEST_UTIL_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TESTS_OPENFHE_TEST_UTIL_H_

#include <array>

#include "absl/memory/memory.h"
#include "gtest/gtest.h"
#include "openfhe/binfhe/binfhecontext.h"
#include "transpiler/data/openfhe_data.h"

constexpr lbcrypto::BINFHE_PARAMSET kSecurityLevel = lbcrypto::MEDIUM;

namespace fully_homomorphic_encryption {
namespace transpiler {

class TranspilerTestBase : public ::testing::Test {
 public:
  const lbcrypto::BinFHEContext cc() const { return cc_; }
  const lbcrypto::LWEPrivateKey sk() const { return sk_; }

 protected:
  static void SetUpTestSuite() {
    cc_.GenerateBinFHEContext(kSecurityLevel);
    sk_ = cc_.KeyGen();
    cc_.BTKeyGen(sk_);
  }

  static void TearDownTestSuite() {
    cc_.ClearBTKeys();
    sk_.reset();
  }

  ~TranspilerTestBase() override {}

 private:
  static lbcrypto::BinFHEContext cc_;
  static lbcrypto::LWEPrivateKey sk_;
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TESTS_OPENFHE_TEST_UTIL_H_
