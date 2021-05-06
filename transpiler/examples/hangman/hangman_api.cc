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

#include "hangman_api.h"

#pragma hls_top
int hangmanMakeMove(char letter) {
  // "hangman" is the secret word that needs to be guessed by the player

  // TODO: Implement a less hacky iterative solution with arrays
  // TODO: Implement support for multiple words

  if (letter == 'h') {
    return 64;  // 100000, Hangman
  }

  if (letter == 'a') {
    return 34;  // 0100010, hAngmAn
  }

  if (letter == 'n') {
    return 17;  // 0010001, haNgmaN
  }

  if (letter == 'g') {
    return 8;  // 0001000, hanGman
  }

  if (letter == 'm') {
    return 4;  // 0000100, hangMan
  }

  // No matching letters
  return 0;
}
