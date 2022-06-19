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

// The main method was split off from the rest of the code to allow testing.
#include <stdlib.h>

#include <bitset>
#include <cassert>
#include <codecvt>
#include <iostream>
#include <locale>
#include <string>

#include "transpiler/data/tfhe_data.h"
#include "transpiler/examples/hangman/hangman_api_tfhe.h"
#include "transpiler/examples/hangman/hangman_client.h"
#include "xls/common/logging/logging.h"

const int MAX_INCORRECT_ATTEMPTS = 6;
const int MAX_WORD_LENGTH = 7;
const int CORRECT_RESULT = 127;

constexpr int kMainMinimumLambda = 120;

int main() {
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  std::cout << "Welcome to a game of hangman." << std::endl;
  int result = 0;
  std::string current_word = "";
  for (int i = 0; i < MAX_WORD_LENGTH; i++) {
    current_word += "_ ";
  }
  int incorrect_attempts_made = 0;

  std::string input;
  // Draw the initial state.
  std::string ascii_result;
  ascii_result = draw_ascii_result(current_word, incorrect_attempts_made);
  std::cout << ascii_result << std::endl;

  // Start the game.
  while (incorrect_attempts_made < MAX_INCORRECT_ATTEMPTS &&
         result != CORRECT_RESULT) {
    std::cout << "Type a letter to make a move: ";
    if (!getline(std::cin, input)) {
      break;
    }
    if (input.length() != 1) {
      std::cout << "Please only enter one letter." << std::endl;

      continue;
    }
    // Encrypt the input.
    auto ciphertext = Tfhe<char>::Encrypt(input[0], key);
    std::cout << "Encryption done" << std::endl;

    std::cout << "Initial state check by decryption: " << std::endl;
    std::cout << ciphertext.Decrypt(key) << "\n";
    std::cout << "\n";

    // Make a move.
    Tfhe<int> cipher_result(params);

    XLS_CHECK_OK(hangmanMakeMove(cipher_result, ciphertext, key.cloud()));

    int move_result = cipher_result.Decrypt(key);
    if (move_result == 0) {
      incorrect_attempts_made += 1;
    } else {
      current_word = update_current_word(input[0], move_result, current_word);
    }
    result = result | move_result;
    ascii_result = draw_ascii_result(current_word, incorrect_attempts_made);
    std::cout << ascii_result << std::endl;
  }
  if (result == CORRECT_RESULT) {
    std::cout << "CONGRATULATIONS!\n" << std::endl;
  } else {
    std::cout << "SORRY, NOT THIS TIME.\n" << std::endl;
  }
  std::cout << "GAME OVER." << std::endl;

  return 0;
}
