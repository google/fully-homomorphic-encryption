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

#include "hangman_client.h"

#include <string>

#include "gtest/gtest.h"
#include "transpiler/examples/hangman/hangman_client.h"

TEST(UpdatesCurrentWordWithLetterTest, Equality) {
  std::string result = update_current_word('h', 64, "_ _ _ _ _ _ _ ");

  EXPECT_EQ(result, "h _ _ _ _ _ _ ");
}

TEST(UpdatesCurrentWordWithMultipleLettersTest, Equality) {
  std::string result = update_current_word('n', 17, "_ _ _ _ _ _ _ ");

  EXPECT_EQ(result, "_ _ n _ _ _ n ");
}

TEST(UpdatesCurrentWordPreservesExistingLettersTest, Equality) {
  std::string result = update_current_word('n', 17, "h _ _ _ _ _ _ ");

  EXPECT_EQ(result, "h _ n _ _ _ n ");
}

TEST(UpdateCurrentWordDoesNotChangeWhenLetterIsMissingTest, Equality) {
  std::string result = update_current_word('h', 0, "_ _ _ _ _ _ _ ");

  EXPECT_EQ(result, "_ _ _ _ _ _ _ ");
}

TEST(DrawAsciiResultInitialStateTest, Equality) {
  std::string result = draw_ascii_result("_ _ _ _ _ _ _ ", 0);

  EXPECT_NE(result.find("_ _ _ _ _ _ _"), std::string::npos);
}

TEST(DrawAsciiResultPreservesLettersTest, Equality) {
  std::string result = draw_ascii_result("h a n _ m a n ", 6);

  EXPECT_NE(result.find("h a n _ m a n"), std::string::npos);
}
