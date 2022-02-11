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

#include "rock_paper_scissor.h"

// Returns "A" if A wins, "B" if B wins, or "=" if it is a tie.
#pragma hls_top
char rock_paper_scissor(char player_a, char player_b) {
  if (player_a == player_b) {
    return '=';
  }
  if ((player_a == 'R') && (player_b == 'S')) {
    return 'A';
  }
  if ((player_a == 'P') && (player_b == 'R')) {
    return 'A';
  }
  if ((player_a == 'S') && (player_b == 'P')) {
    return 'A';
  }
  return 'B';
}
