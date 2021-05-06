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

#include <stdlib.h>

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cctype>
#include <codecvt>
#include <iostream>
#include <locale>
#include <string>

const int MAX_WORD_LENGTH = 7;
const std::string LINE_SEPARATOR =
    "================================================";

std::string update_current_word(char input_letter, int move_result,
                                std::string current_word) {
  std::string bitwise_result =
      std::bitset<MAX_WORD_LENGTH>(move_result).to_string();
  std::string updated_word = current_word;
  for (int i = 0; i < MAX_WORD_LENGTH; i++) {
    if (bitwise_result[i] == '1') {
      // multiply i by 2 to account for empty space between letters.
      updated_word[2 * i] = input_letter;
    }
  }
  return updated_word;
}

// This code is merely meant for visualization. It does not really reflect what
// the server sees.
std::string generate_gibberish(int length) {
  std::string result = "";
  for (int i = 0; i < length; i++) {
    char c;
    do {
      c = rand() % 256;
    } while (!std::isgraph(c));
    result += c;
  }
  return result;
}

std::string draw_ascii_result(std::string current_word,
                              int incorrect_attempts_made) {
  std::string gallows_top =
      std::string("-----CLIENT VIEW ----- | -----SERVER VIEW -----\n") +
      std::string("  -------              | ------- \n") +
      std::string(" |  /  |               | |  /  |\n | / ");
  std::string gallows_bottom =
      std::string(" |                     | |\n") +
      std::string(" |----------           | |----------\n") +
      std::string(" |         |           | |         |\n") +
      std::string(" |_________|           | |_________|\n\n");

  std::string results[7] = {
      "                  | | / " + generate_gibberish(7) + "\n" +
          " |                     | |   " + generate_gibberish(7) + "\n" +
          " |                     | |   " + generate_gibberish(7) + "\n",
      "  O               | | / " + generate_gibberish(7) + "\n" +
          " |                     | |   " + generate_gibberish(7) + "\n" +
          " |                     | |   " + generate_gibberish(7) + "\n",
      "  O               | | / " + generate_gibberish(7) + "\n" +
          " |     |               | |   " + generate_gibberish(7) + "\n" +
          " |                     | |   " + generate_gibberish(7) + "\n",
      "  O               | | / " + generate_gibberish(7) + "\n" +
          " |    /|               | |   " + generate_gibberish(7) + "\n" +
          " |                     | |   " + generate_gibberish(7) + "\n",
      "  O               | | / " + generate_gibberish(7) + "\n" +
          " |    /|\\              | |   " + generate_gibberish(7) + "\n" +
          " |                     | |   " + generate_gibberish(7) + "\n",
      "  O               | | / " + generate_gibberish(7) + "\n" +
          " |    /|\\              | |   " + generate_gibberish(7) + "\n" +
          " |    /                | |   " + generate_gibberish(7) + "\n",
      "  O               | | / " + generate_gibberish(7) + "\n" +
          " |    /|\\              | |   " + generate_gibberish(7) + "\n" +
          " |    / \\              | |   " + generate_gibberish(7) + "\n"};
  std::string ascii_result = "";

  ascii_result += LINE_SEPARATOR + '\n';
  ascii_result += gallows_top;
  ascii_result += results[incorrect_attempts_made];
  ascii_result += gallows_bottom;
  ascii_result += LINE_SEPARATOR + '\n';

  std::string current_server_word = "";
  for (int i = 0; i < MAX_WORD_LENGTH; i++) {
    current_server_word += generate_gibberish(1) + " ";
  }

  ascii_result += "Current guess:         |  Current guess:\n";
  ascii_result += current_word + "         |  " + current_server_word + "\n";
  ascii_result += LINE_SEPARATOR + '\n';

  return ascii_result;
}
