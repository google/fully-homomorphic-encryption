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

// Returns a (potentially empty) bitmask of positions of the provided letter
// in the secret word. This allows building a server that allows playing hangman
// without knowing which moves the players make and whether they win or lose.
//
// Example: given the letter "a" as an input, return value of 34 (0100010)
// reveals two letter positions (hAngmAn).
//
// Expects lowercase characters.
// Assumes that the player knows the length of the secret word.

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_HANGMAN_HANGMAN_API_H
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_HANGMAN_HANGMAN_API_H

int hangmanMakeMove(char letter);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_HANGMAN_HANGMAN_API_H
