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
#include "transpiler/tests/namespaced_struct_tfhe.h"
#include "transpiler/tests/struct2_tfhe.h"
#include "transpiler/tests/struct_tfhe.h"
#include "transpiler/tests/tfhe_test_util.h"
#include "xls/common/status/matchers.h"

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

using ::fully_homomorphic_encryption::transpiler::TranspilerTestBase;
using ::outer::inner::kSumSimpleArraySize;
using ::outer::inner::Simple;

class TranspilerTypesTest : public TranspilerTestBase {};

TEST_F(TranspilerTypesTest, TestArray) {
  auto ciphertext = TfheArray<char>::Encrypt("abcd", secret_key());
  TfheArray<char> result(4, params());
  XLS_ASSERT_OK(test_array(ciphertext, result, cloud_key()));
  EXPECT_EQ(result.Decrypt(secret_key()), "acce");
}

TEST_F(TranspilerTypesTest, TestChar) {
  auto ciphertext = Tfhe<char>::Encrypt('a', secret_key());
  Tfhe<char> result(params());
  XLS_ASSERT_OK(test_char(result, ciphertext, cloud_key()));
  EXPECT_EQ(result.Decrypt(secret_key()), 'b');
}

TEST_F(TranspilerTypesTest, TestInt) {
  auto ciphertext = Tfhe<int>::Encrypt(100, secret_key());
  Tfhe<int> result(params());
  XLS_ASSERT_OK(test_int(result, ciphertext, cloud_key()));
  EXPECT_EQ(result.Decrypt(secret_key()), 101);
}

TEST_F(TranspilerTypesTest, TestLong) {
  auto ciphertext = Tfhe<long>::Encrypt(100, secret_key());
  Tfhe<long> result(params());
  XLS_ASSERT_OK(test_long(result, ciphertext, cloud_key()));
  EXPECT_EQ(result.Decrypt(secret_key()), 101);
}

TEST_F(TranspilerTypesTest, TestStruct) {
  Struct value = {'b', (short)0x5678, (int)0xc0deba7e};
  auto ciphertext = Tfhe<Struct>::Encrypt(value, secret_key());
  XLS_ASSERT_OK(my_package(ciphertext, cloud_key()));

  Struct result = ciphertext.Decrypt(secret_key());
  EXPECT_EQ(value.c, result.c);
  EXPECT_EQ(value.i, result.i);
  EXPECT_EQ(value.s, result.s);
}

TEST_F(TranspilerTypesTest, TestMultipleFunctionsOneStruct) {
  Struct value = {'b', (short)0x5678, (int)0xc0deba7e};
  auto ciphertext = Tfhe<Struct>::Encrypt(value, secret_key());
  XLS_ASSERT_OK(my_package(ciphertext, cloud_key()));
  XLS_ASSERT_OK(function2(ciphertext, cloud_key()));

  Struct result = ciphertext.Decrypt(secret_key());
  EXPECT_EQ(value.c, result.c);
  EXPECT_EQ(value.i, result.i);
  EXPECT_EQ(value.s, result.s);
}

TEST_F(TranspilerTypesTest, TestArrayOfNamespacedStructs) {
  Simple data[kSumSimpleArraySize] = {{1}, {2}, {3}};
  auto ciphertext =
      TfheArray<Simple, kSumSimpleArraySize>::Encrypt(data, secret_key());

  // Call into circuit that takes an array of `tests::Simple` to sum
  Tfhe<unsigned short> result_ciphertext(params());
  XLS_ASSERT_OK(sum_simple_structs(result_ciphertext, ciphertext, cloud_key()));

  // Decrypt and verify
  unsigned short result = result_ciphertext.Decrypt(secret_key());
  EXPECT_EQ(result, 6);
}

}  // namespace
}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
