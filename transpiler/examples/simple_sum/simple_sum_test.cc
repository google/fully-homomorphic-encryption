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

#include "simple_sum.h"

#include <array>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/data/tfhe_value.h"
#include "transpiler/examples/simple_sum/simple_sum.h"
#include "xls/common/status/matchers.h"

#if defined(USE_YOSYS_INTERPRETED_TFHE)
#include "transpiler/examples/simple_sum/simple_sum_yosys_interpreted_tfhe.h"
#else
#include "transpiler/examples/simple_sum/simple_sum_tfhe.h"
#endif

TEST(SimpleSumTest, Basic) {
  int x = 64;
  int y = 45;
  int result = simple_sum(x, y);

  EXPECT_EQ(result, 109);
}

TEST(SimpleSumTest, NegativeValue) {
  int x = 64;
  int y = -45;
  int result = simple_sum(x, y);

  EXPECT_EQ(result, 19);
}

TEST(SimpleSumTest, TwoNegativeValues) {
  int x = -64;
  int y = -45;
  int result = simple_sum(x, y);

  EXPECT_EQ(result, -109);
}

TEST(SimpleSumTest, CorrectlyAddsEncryptedIntegers) {
  constexpr int kMainMinimumLambda = 120;
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  auto ciphertext_x = Tfhe<int>::Encrypt(2, key);
  auto ciphertext_y = Tfhe<int>::Encrypt(3, key);
  Tfhe<int> cipher_result(params);
  XLS_ASSERT_OK(
      simple_sum(cipher_result, ciphertext_x, ciphertext_y, key.cloud()));

  EXPECT_EQ(ciphertext_x.Decrypt(key), 2);
  EXPECT_EQ(ciphertext_y.Decrypt(key), 3);
  EXPECT_EQ(cipher_result.Decrypt(key), 5);
}
