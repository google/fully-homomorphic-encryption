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

#include "string_cap_char.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/codelab/string_cap_char/string_cap_char_tfhe.h"
#include "transpiler/data/tfhe_data.h"
#include "xls/common/status/matchers.h"

TEST(StringCapCharTest, CorrectlyCapitalizesShortPhrase) {
  State st;
  const char* input = "do or do not";
  std::string result = "";
  char inc;
  while ((inc = *input++)) {
    result += st.process(inc);
  }

  EXPECT_EQ(result, "Do Or Do Not");
}

TEST(StringCapCharTest, CorrectlyCapitalizesLongPhrase) {
  State st;
  const char* input = "do or do not; there is no try!.!";
  std::string result = "";
  char inc;
  while ((inc = *input++)) {
    result += st.process(inc);
  }

  EXPECT_EQ(result, "Do Or Do Not; There Is No Try!.!");
}

TEST(StringCapCharTest, CorrectlyProcessesSpecialChars) {
  State st;
  const char* input = "d,o o.r^ d&*::o no!t;";
  std::string result = "";
  char inc;
  while ((inc = *input++)) {
    result += st.process(inc);
  }

  EXPECT_EQ(result, "D,o O.r^ D&*::o No!t;");
}

TEST(StringCapCharTest, CorrectlyCapitalizesLongPhraseEncrypted) {
  constexpr int kMainMinimumLambda = 120;
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  std::string plaintext("do or do not; there is no try!.!");
  int32_t data_size = plaintext.size();

  auto ciphertext = TfheArray<char>::Encrypt(plaintext, key);
  TfheArray<char> cipher_result(data_size, params);

  Tfhe<State> state(params);
  state.SetUnencrypted(State(), key.cloud());
  for (int i = 0; i < data_size; i++) {
    XLS_ASSERT_OK(
        my_package(cipher_result[i], state, ciphertext[i], key.cloud()));
  }
  std::string result = cipher_result.Decrypt(key);

  EXPECT_EQ(result, "Do Or Do Not; There Is No Try!.!");
}
