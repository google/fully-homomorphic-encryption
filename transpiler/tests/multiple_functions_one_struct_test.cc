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

#include <ios>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/tests/openfhe_test_util.h"
#include "transpiler/tests/struct.h"
#include "transpiler/tests/struct2_openfhe.h"
#include "transpiler/tests/struct_openfhe.h"
#include "xls/common/status/matchers.h"

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

using ::fully_homomorphic_encryption::transpiler::TranspilerTestBase;

class MultipleFunctionsOneStructTest : public TranspilerTestBase {};

// cc(), sk()
TEST_F(MultipleFunctionsOneStructTest, FunctionsInSequence) {
  Struct value = {'b', (short)0x5678, (int)0xc0deba7e};

  OpenFhe<Struct> encrypted_value = OpenFhe<Struct>::Encrypt(value, cc(), sk());

  XLS_ASSERT_OK(function2(encrypted_value, cc()));
  XLS_ASSERT_OK(my_package(encrypted_value, cc()));

  Struct result = encrypted_value.Decrypt(sk());
  EXPECT_EQ(value.c, result.c);
  EXPECT_EQ(value.i, result.i);
  EXPECT_EQ(value.s, result.s);
}

}  // namespace
}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
