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
#include "absl/strings/numbers.h"
#include "transpiler/data/cleartext_data.h"
#include "xls/common/logging/logging.h"
#ifdef USE_YOSYS_CLEARTEXT
#include "transpiler/examples/ac_int/ac_int_ops_yosys_cleartext.h"
#else
#include "transpiler/examples/ac_int/ac_int_ops_cleartext.h"
#endif

Encoded<XlsInt<22, false>> TimedSqrt(Encoded<XlsInt<22, false>>& ciphertext) {
  double start_time = clock();
  std::cout << "Starting!" << std::endl;
  Encoded<XlsInt<22, false>> result;
  XLS_CHECK_OK(isqrt(result, ciphertext));
  std::cout << "\t\t\t\t\tTotal time "
            << ": " << (clock() - start_time) / 1000000 << " secs" << std::endl;
  return result;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: sqrt_cleartext_testbench num\n\n");
    return 1;
  }

  std::string input = argv[1];

  int in;
  XLS_CHECK(absl::SimpleAtoi(input, &in));
  ac_int<22, false> plaintext(in);
  std::cout << "plaintext: " << plaintext << std::endl;

  // Encode data
  Encoded<XlsInt<22, false>> ciphertext(plaintext);
  std::cout << "Encoding done" << std::endl;

  std::cout << "Initial state check by decoding: " << std::endl;
  // Decode results.
  std::cout << ac_int<22, false>(ciphertext.Decode()) << "\n";

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Compute the square root.
  Encoded<XlsInt<22, false>> result = TimedSqrt(ciphertext);
  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  // Decode results.
  std::cout << "Decoded result: " << ac_int<22, false>(result.Decode()) << "\n";
  std::cout << "Decoding done" << std::endl;
}
