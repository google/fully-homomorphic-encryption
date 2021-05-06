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

#pragma hls_top
void CapitalizeString(char my_string[MAX_LENGTH]) {
  bool last_was_space = true;
#pragma hls_unroll yes
  for (int i = 0; i < MAX_LENGTH; i++) {
    char c = my_string[i];
    if (last_was_space && c >= 'a' && c <= 'z') {
      my_string[i] = c - ('a' - 'A');
    }
    last_was_space = (c == ' ');
  }
}
