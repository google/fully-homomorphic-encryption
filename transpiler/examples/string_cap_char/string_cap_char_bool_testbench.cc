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
#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "absl/container/fixed_array.h"
#include "transpiler/data/boolean_data.h"
#include "transpiler/examples/string_cap_char/string_cap_char_bool.h"
#include "xls/common/logging/logging.h"

void BoolStringCap(EncodedString& cipherresult, EncodedString& ciphertext,
                   int data_size, EncodedState& state) {
  time_t start_time;
  time_t end_time;
  double char_time = 0.0;
  double total_time = 0.0;
  for (int i = 0; i < data_size; i++) {
    start_time = clock();
    XLS_CHECK_OK(my_package(cipherresult[i], state, ciphertext[i]));
    end_time = clock();
    char_time = (end_time - start_time);
    std::cout << "\t\t\t\t\tchar " << i << ": " << char_time / 1000000
              << " secs" << std::endl;
    total_time += char_time;
  }
  std::cout << "\t\t\t\t\tTotal time "
            << ": " << total_time / 1000000 << " secs" << std::endl;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: string_cap_bool_testbench string_input\n\n");
    return 1;
  }

  const char* input = argv[1];

  std::string plaintext(input);
  int data_size = plaintext.size();
  std::cout << "plaintext(" << data_size << "):" << plaintext << std::endl;

  // Encode data
  EncodedString ciphertext(plaintext);
  std::cout << "Encoding done" << std::endl;

  std::cout << "Initial state check by decoding: " << std::endl;
  // Decode results.
  std::cout << ciphertext.Decode() << "\n";

  State st;
  EncodedState cipherstate;
  cipherstate.Encode(st);
  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Perform string capitalization
  EncodedString cipher_result(data_size);
  BoolStringCap(cipher_result, ciphertext, data_size, cipherstate);
  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  std::cout << "Decoded result: ";
  // Decode results.
  std::cout << cipher_result.Decode() << "\n";
  std::cout << "Decoding done" << std::endl;
}
