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
#include "transpiler/data/openfhe_data.h"
#include "transpiler/tests/array_openfhe.h"
#include "transpiler/tests/char_openfhe.h"
#include "transpiler/tests/int_openfhe.h"
#include "transpiler/tests/long_openfhe.h"
#include "transpiler/tests/namespaced_struct_openfhe.h"
#include "transpiler/tests/openfhe_test_util.h"
#include "transpiler/tests/struct2_openfhe.h"
#include "transpiler/tests/struct_openfhe.h"
#include "xls/common/status/matchers.h"

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

using ::fully_homomorphic_encryption::transpiler::TranspilerTestBase;
using ::outer::inner::kSumSimpleArraySize;
using ::outer::inner::Simple;

class TranspilerTypesTest : public TranspilerTestBase {};

TEST_F(TranspilerTypesTest, TestArray) {
  auto ciphertext = OpenFheArray<char>::Encrypt("abcd", cc(), sk());
  OpenFheArray<char> result(4, cc());
  XLS_ASSERT_OK(test_array(ciphertext, result, cc()));
  EXPECT_EQ(result.Decrypt(sk()), "acce");
}

TEST_F(TranspilerTypesTest, TestChar) {
  auto ciphertext = OpenFhe<char>::Encrypt('a', cc(), sk());
  OpenFhe<char> result(cc());
  XLS_ASSERT_OK(test_char(result, ciphertext, cc()));
  EXPECT_EQ(result.Decrypt(sk()), 'b');
}

TEST_F(TranspilerTypesTest, TestInt) {
  auto ciphertext = OpenFhe<int>::Encrypt(100, cc(), sk());
  OpenFhe<int> result(cc());
  XLS_ASSERT_OK(test_int(result, ciphertext, cc()));
  EXPECT_EQ(result.Decrypt(sk()), 101);
}

TEST_F(TranspilerTypesTest, TestLong) {
  auto ciphertext = OpenFhe<long>::Encrypt(100, cc(), sk());
  OpenFhe<long> result(cc());
  XLS_ASSERT_OK(test_long(result, ciphertext, cc()));
  EXPECT_EQ(result.Decrypt(sk()), 101);
}

TEST_F(TranspilerTypesTest, TestStruct) {
  Struct value = {'b', (short)0x5678, (int)0xc0deba7e};

  OpenFhe<Struct> encrypted_value = OpenFhe<Struct>::Encrypt(value, cc(), sk());
  XLS_ASSERT_OK(my_package(encrypted_value, cc()));

  Struct result = encrypted_value.Decrypt(sk());
  EXPECT_EQ(value.c, result.c);
  EXPECT_EQ(value.i, result.i);
  EXPECT_EQ(value.s, result.s);
}

TEST_F(TranspilerTypesTest, TestMultipleFunctionsOneStruct) {
  Struct value = {'b', (short)0x5678, (int)0xc0deba7e};

  OpenFhe<Struct> encrypted_value = OpenFhe<Struct>::Encrypt(value, cc(), sk());
  XLS_ASSERT_OK(my_package(encrypted_value, cc()));
  XLS_ASSERT_OK(function2(encrypted_value, cc()));

  Struct result = encrypted_value.Decrypt(sk());
  EXPECT_EQ(value.c, result.c);
  EXPECT_EQ(value.i, result.i);
  EXPECT_EQ(value.s, result.s);
}

TEST_F(TranspilerTypesTest, TestArrayOfNamespacedStructs) {
  Simple data[kSumSimpleArraySize] = {{1}, {2}, {3}};
  auto ciphertext = OpenFheArray<Simple, kSumSimpleArraySize>(cc());
  ciphertext.SetEncrypted(data, sk());

  // Call into circuit that takes an array of `tests::Simple` to sum
  OpenFhe<unsigned short> result_ciphertext(cc());
  XLS_ASSERT_OK(sum_simple_structs(result_ciphertext, ciphertext, cc()));

  // Decrypt and verify
  unsigned short result = result_ciphertext.Decrypt(sk());
  EXPECT_EQ(result, 6);
}

}  // namespace
}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
