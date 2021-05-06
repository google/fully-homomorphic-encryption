//
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
//

#include "string_reverse.h"

int StrLen(char my_string[MAX_LENGTH]) {
  int len = MAX_LENGTH;

#pragma hls_unroll yes
  for (int i = MAX_LENGTH - 1; i >= 0; i--) {
    if (my_string[i] == '\0') {
      len = i;
    }
  }

  return len;
}

#pragma hls_top
void ReverseString(char my_string[MAX_LENGTH]) {
  const int len = StrLen(my_string);
  const int half_len = len / 2;

#pragma hls_unroll yes
  for (int i = 0; i < MAX_LENGTH; i++) {
    if (i < half_len) {
      const char temp = my_string[i];
      my_string[i] = my_string[len - i - 1];
      my_string[len - i - 1] = temp;
    }
  }
}
