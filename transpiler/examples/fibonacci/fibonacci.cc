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

#include "fibonacci.h"

#pragma hls_top
int fibonacci_number(int n) {
  if (n < 0 || n > 10) {
    return -1;
  } else if (n == 0) {
    return 0;
  } else if (n == 1) {
    return 1;
  }

  long prev = 0;
  long cur = 1;
  long temp = 0;
#pragma hls_unroll yes
  for (int i = 2; i <= 10; i++) {
    temp = cur;
    cur = prev + cur;
    prev = temp;
    if (i == n) {
      return cur;
    }
  }

  // Normally this statement shouldn't be reached. If it was reached, it means
  // there was an issue.
  return -2;
}
