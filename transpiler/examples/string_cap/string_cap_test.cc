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

#include "string_cap.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/data/tfhe_data.h"
#include "transpiler/examples/string_cap/string_cap.h"
#include "xls/common/status/matchers.h"

#if defined(USE_INTERPRETED_TFHE)
#include "transpiler/examples/string_cap/string_cap_tfhe_xls_interpreted.h"
#else
#include "transpiler/examples/string_cap/string_cap_tfhe_xls_transpiled.h"
#endif

TEST(StringCapTest, CorrectlyCapitalizesShortPhrase) {
  char result[MAX_LENGTH] = "do or do not";
  CapitalizeString(result);

  ASSERT_STREQ(result, "Do Or Do Not");
}

TEST(StringCapTest, CorrectlyCapitalizesLongPhrase) {
  char result[] = "do or do not; there is no try!.!";
  CapitalizeString(result);

  ASSERT_STREQ(result, "Do Or Do Not; There Is No Try!.!");
}

TEST(StringCapTest, CorrectlyCapitalizesLongPhraseEncrypted) {
  constexpr int kMainMinimumLambda = 120;
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  char input[] = "do or do not; there is no try!.!";
  auto ciphertext = TfheArray<char>::Encrypt(input, key);
  XLS_ASSERT_OK(CapitalizeString(ciphertext, key.cloud()));

  std::string result = ciphertext.Decrypt(key);
  // Remove the last null terminator for comparisons.
  result.erase(result.find('\0'));

  EXPECT_EQ(result, "Do Or Do Not; There Is No Try!.!");
}

TEST(StringCapTest, CorrectlyProcessesSpecialChars) {
  char result[MAX_LENGTH] = "d,o o.r^ d&*::o no!t;";
  CapitalizeString(result);

  ASSERT_STREQ(result, "D,o O.r^ D&*::o No!t;");
}
