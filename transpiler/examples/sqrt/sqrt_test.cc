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

#include "sqrt.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/data/tfhe_data.h"
#include "transpiler/examples/sqrt/sqrt_tfhe.h"
#include "xls/common/status/matchers.h"

TEST(SqrtTest, SmallPerfectSquare) { ASSERT_EQ(isqrt(25), 5); }

TEST(SqrtTest, SmallSquareRoot) { ASSERT_EQ(isqrt(35), 5); }

TEST(SqrtTest, LargePerfectSquare) { ASSERT_EQ(isqrt(22'500), 150); }

TEST(SqrtTest, LargeSquareRoot) { ASSERT_EQ(isqrt(22'800), 150); }

TEST(SqrtTest, SmallPerfectSquareEncrypted) {
  constexpr int kMainMinimumLambda = 120;
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  auto ciphertext = Tfhe<short>::Encrypt(25, key);
  Tfhe<short> result(key.params());
  XLS_ASSERT_OK(isqrt(result, ciphertext, key.cloud()));

  ASSERT_EQ(result.Decrypt(key), 5);
}

TEST(SqrtTest, SmallSquareRootEncrypted) {
  constexpr int kMainMinimumLambda = 120;
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  auto ciphertext = Tfhe<short>::Encrypt(35, key);
  Tfhe<short> result(key.params());
  XLS_ASSERT_OK(isqrt(result, ciphertext, key.cloud()));

  ASSERT_EQ(result.Decrypt(key), 5);
}

TEST(SqrtTest, LargePerfectSquareEncrypted) {
  constexpr int kMainMinimumLambda = 120;
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  auto ciphertext = Tfhe<short>::Encrypt(22'500, key);
  Tfhe<short> result(key.params());
  XLS_ASSERT_OK(isqrt(result, ciphertext, key.cloud()));

  ASSERT_EQ(result.Decrypt(key), 150);
}

TEST(SqrtTest, LargeSquareRootEncrypted) {
  constexpr int kMainMinimumLambda = 120;
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  auto ciphertext = Tfhe<short>::Encrypt(22'800, key);
  Tfhe<short> result(key.params());
  XLS_ASSERT_OK(isqrt(result, ciphertext, key.cloud()));

  ASSERT_EQ(result.Decrypt(key), 150);
}
