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
  auto ciphertext = TfheString::Encrypt("abcd", secret_key());
  TfheString result(4, params());
  XLS_ASSERT_OK(test_array(ciphertext, result, cloud_key()));
  EXPECT_EQ(result.Decrypt(secret_key()), "acce");
}

TEST_F(TranspilerTypesTest, TestChar) {
  auto ciphertext = TfheChar::Encrypt('a', secret_key());
  TfheChar result(params());
  XLS_ASSERT_OK(test_char(result, ciphertext, cloud_key()));
  EXPECT_EQ(result.Decrypt(secret_key()), 'b');
}

TEST_F(TranspilerTypesTest, TestInt) {
  auto ciphertext = TfheInt::Encrypt(100, secret_key());
  TfheInt result(params());
  XLS_ASSERT_OK(test_int(result, ciphertext, cloud_key()));
  EXPECT_EQ(result.Decrypt(secret_key()), 101);
}

TEST_F(TranspilerTypesTest, TestLong) {
  auto ciphertext = TfheLong::Encrypt(100, secret_key());
  TfheLong result(params());
  XLS_ASSERT_OK(test_long(result, ciphertext, cloud_key()));
  EXPECT_EQ(result.Decrypt(secret_key()), 101);
}

}  // namespace
}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
