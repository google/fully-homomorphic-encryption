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

#include "fibonacci_sequence.h"  // NOLINT: xlscc currently needs locally-scoped includes.

#pragma hls_top
void fibonacci_sequence(int n, int output[FIBONACCI_SEQUENCE_SIZE]) {
  if (n < 0 || n > 10) {
    return;
  }

  int prev = 0;
  int cur = 1;
  int temp = 0;

// Go through the Fibonacci sequence until n is reached.
#pragma hls_unroll yes
  for (int i = 2; i <= 10; i++) {
    // We need to iterate over the Fibonacci sequence only until we reach
    // n'th value.
    if (i <= n) {
      temp = cur;
      cur = prev + cur;
      prev = temp;
    }
  }

  // Fill-in the output.
  if (n == 0) {
    output[0] = 0;
  } else {
    output[0] = cur;
  }
#pragma hls_unroll yes
  for (int i = 1; i < FIBONACCI_SEQUENCE_SIZE; i++) {
    temp = cur;
    cur = prev + cur;
    prev = temp;
    output[i] = cur;
  }
}
