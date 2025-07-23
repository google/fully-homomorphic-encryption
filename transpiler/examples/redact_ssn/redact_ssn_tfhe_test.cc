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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redact_ssn.h"
#include "transpiler/data/tfhe_data.h"
#include "transpiler/examples/redact_ssn/redact_ssn.h"
#include "transpiler/examples/redact_ssn/redact_ssn_tfhe_yosys_interpreted.h"
#include "xls/common/status/matchers.h"

TEST(RedactSsnTest, RedactSsnEncrypted) {
  constexpr int kMainMinimumLambda = 120;
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  std::string input = "redact 123456789 away";
  input.resize(MAX_LENGTH, '\0');
  auto ciphertext = TfheArray<char>::Encrypt(input, key);
  XLS_ASSERT_OK(RedactSsn(ciphertext, key.cloud()));

  std::string result = ciphertext.Decrypt(key);
  // Remove the last null terminator for comparisons.
  result.erase(result.find('\0'));

  EXPECT_EQ(result, "redact ********* away");
}
