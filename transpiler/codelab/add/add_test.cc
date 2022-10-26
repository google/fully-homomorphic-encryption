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

#include "gtest/gtest.h"
#include "transpiler/codelab/add/add_fhe_lib.h"
#include "transpiler/data/openfhe_data.h"

TEST(AddOpenFheTest, Sum) {
  int x = 5, y = 7;

  auto context = lbcrypto::BinFHEContext();
  context.GenerateBinFHEContext(lbcrypto::MEDIUM);
  auto sk = context.KeyGen();
  context.BTKeyGen(sk);

  auto ciphertext_x = OpenFhe<signed int>::Encrypt(x, context, sk);
  auto ciphertext_y = OpenFhe<signed int>::Encrypt(y, context, sk);
  OpenFhe<signed int> result(context);
  XLS_CHECK_OK(add(result, ciphertext_x, ciphertext_y, context));

  int result_plaintext = result.Decrypt(sk);
  EXPECT_EQ(result_plaintext, 12);
}
