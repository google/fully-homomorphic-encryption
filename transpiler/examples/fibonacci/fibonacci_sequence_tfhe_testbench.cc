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

#include <stdio.h>
#include <time.h>

#include <array>
#include <iostream>
#include <string>
#include <vector>

#include "absl/strings/str_format.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
#include "transpiler/data/tfhe_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_TFHE
#include "transpiler/examples/fibonacci/fibonacci_sequence_interpreted_tfhe.h"
#else
#include "transpiler/examples/fibonacci/fibonacci_sequence_tfhe.h"
#endif

const int kMainMinimumLambda = 120;

int main(int argc, char** argv) {
  // Generate a keyset.
  TFHEParameters params(kMainMinimumLambda);

  // Generate a random key.
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  // Create inputs.
  int input = 5;
  auto encrypted_input = TfheValue<int>::Encrypt(input, key);
  std::cout << absl::StreamFormat("Decrypted input: %d",
                                  encrypted_input.Decrypt(key))
            << std::endl;

  TfheArray<int> encrypted_result(500, params);
  XLS_CHECK_OK(fibonacci_sequence(encrypted_input.get(), encrypted_result.get(),
                                  key.cloud()));
  absl::FixedArray<int> result = encrypted_result.Decrypt(key);
  std::cout << absl::StrFormat("Result: %d, %d, %d, %d, %d", result[0],
                               result[1], result[2], result[3], result[4])
            << std::endl;

  return 0;
}
