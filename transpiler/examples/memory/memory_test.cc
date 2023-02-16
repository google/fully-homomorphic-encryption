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

#include "transpiler/examples/memory/memory.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/data/openfhe_data.h"
#include "transpiler/examples/memory/memory_yosys_interpreted_openfhe.h"
#include "xls/common/status/matchers.h"

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

TEST(MemoryTest, RunsWithoutError) {
  auto cc = lbcrypto::BinFHEContext();
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  int8_t input[4][4] = {
      {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15}};

  auto ciphertext = OpenFheArray<int8_t, 4, 4>(cc);
  ciphertext.SetEncrypted(input, sk);
  // Just check it runs without error, because this test is primarily
  // to ensure yosys doesn't include memory cells in its output, which
  // the input code triggers if the bug fix regresses.
  XLS_ASSERT_OK(Memory(ciphertext, cc));
}

}  // namespace
}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
