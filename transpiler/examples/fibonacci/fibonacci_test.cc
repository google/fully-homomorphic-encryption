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

#include <string>

#include "fibonacci_sequence.h"
#include "gmock/gmock.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
#include "transpiler/data/tfhe_data.h"
#include "transpiler/examples/fibonacci/fibonacci_sequence_tfhe.h"
#include "xls/common/status/matchers.h"

TEST(FibonacciTest, InitialState) {
  int expected_result[5] = {1, 1, 2, 3, 5};
  int actual_result[5] = {0};

  fibonacci_sequence(1, actual_result);

  EXPECT_THAT(actual_result, testing::ElementsAreArray(expected_result));
}

TEST(FibonacciTest, MidState) {
  int expected_result[5] = {2, 3, 5, 8, 13};
  int actual_result[5] = {0};

  fibonacci_sequence(3, actual_result);

  EXPECT_THAT(actual_result, testing::ElementsAreArray(expected_result));
}

TEST(FibonacciTest, EndState) {
  int expected_result[5] = {55, 89, 144, 233, 377};
  int actual_result[5] = {0};

  fibonacci_sequence(10, actual_result);

  EXPECT_THAT(actual_result, testing::ElementsAreArray(expected_result));
}

TEST(FibonacciTest, OutOfBounds) {
  int expected_result[5] = {0};
  int actual_result[5] = {0};
  // Values larger than 10 are not supported.
  fibonacci_sequence(11, actual_result);

  EXPECT_THAT(actual_result, testing::ElementsAreArray(expected_result));
}

TEST(FibonacciTest, InitialStateEncrypted) {
  const int kMainMinimumLambda = 120;

  // Generate a keyset.
  TFHEParameters params(kMainMinimumLambda);

  // Generate a random key.
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  // Create inputs.
  int input = 5;
  auto encrypted_input = Tfhe<int>::Encrypt(input, key);

  TfheArray<int> encrypted_result(5, params);
  XLS_ASSERT_OK(
      fibonacci_sequence(encrypted_input, encrypted_result, key.cloud()));
  absl::FixedArray<int> actual_result = encrypted_result.Decrypt(key);

  int expected_result[5] = {5, 8, 13, 21, 34};

  EXPECT_THAT(actual_result, testing::ElementsAreArray(expected_result));
}
