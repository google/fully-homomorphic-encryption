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

#include <array>
#include <initializer_list>

#include "absl/container/fixed_array.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/data/tfhe_data.h"
#include "transpiler/tests/array_tfhe.h"
#include "transpiler/tests/char_tfhe.h"
#include "transpiler/tests/int_tfhe.h"
#include "transpiler/tests/long_tfhe.h"
#include "transpiler/tests/test_util.h"
#include "xls/common/status/matchers.h"

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

using ::fully_homomorphic_encryption::transpiler::TranspilerTestBase;

class TranspilerTypesTest : public TranspilerTestBase {};

TEST_F(TranspilerTypesTest, TestArray) {
  auto ciphertext = TfheArray<char>::Encrypt("abcd", secret_key());
  TfheArray<char> result(4, params());
  XLS_ASSERT_OK(test_array(ciphertext.get(), result.get(), cloud_key()));
  std::initializer_list<char> expected{'a', 'c', 'c', 'e'};
  EXPECT_EQ(result.Decrypt(secret_key()), absl::FixedArray<char>(expected));
}

TEST_F(TranspilerTypesTest, TestChar) {
  auto ciphertext = TfheValue<char>::Encrypt('a', secret_key());
  TfheValue<char> result(params());
  XLS_ASSERT_OK(test_char(result.get(), ciphertext.get(), cloud_key()));
  EXPECT_EQ(result.Decrypt(secret_key()), 'b');
}

TEST_F(TranspilerTypesTest, TestInt) {
  auto ciphertext = TfheValue<int>::Encrypt(100, secret_key());
  TfheValue<int> result(params());
  XLS_ASSERT_OK(test_int(result.get(), ciphertext.get(), cloud_key()));
  EXPECT_EQ(result.Decrypt(secret_key()), 101);
}

TEST_F(TranspilerTypesTest, TestLong) {
  auto ciphertext = TfheValue<long>::Encrypt(100, secret_key());
  TfheValue<long> result(params());
  XLS_ASSERT_OK(test_long(result.get(), ciphertext.get(), cloud_key()));
  EXPECT_EQ(result.Decrypt(secret_key()), 101);
}

}  // namespace
}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
