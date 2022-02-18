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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_HANGMAN_HANGMAN_CLIENT_H
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_HANGMAN_HANGMAN_CLIENT_H

#include <string>

#include "transpiler/examples/hangman/hangman_api.h"

std::string update_current_word(char input_letter, int move_result,
                                std::string current_word);

std::string draw_ascii_result(std::string current_word,
                              int incorrect_attempts_made);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_HANGMAN_HANGMAN_CLIENT_H
