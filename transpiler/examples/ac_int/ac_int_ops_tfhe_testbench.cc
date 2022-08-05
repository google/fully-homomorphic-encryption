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

#include "absl/strings/numbers.h"
#include "transpiler/data/fhe_xls_int.h"
#include "transpiler/data/tfhe_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_TFHE
#include "transpiler/examples/ac_int/ac_int_ops_interpreted_tfhe.h"
#elif USE_YOSYS_INTERPRETED_TFHE
#include "transpiler/examples/ac_int/ac_int_ops_yosys_interpreted_tfhe.h"
#else
#include "transpiler/examples/ac_int/ac_int_ops_tfhe.h"
#endif

constexpr int kMainMinimumLambda = 120;

Tfhe<XlsInt<22, false>> TfheSqrt(TfheRef<XlsInt<22, false>> ciphertext,
                                 TFHESecretKeySet& key) {
  absl::Time start_time = absl::Now();
  double cpu_start_time = clock();
  std::cout << "Starting!" << std::endl;
  Tfhe<XlsInt<22, false>> result(key.cloud()->params);
  XLS_CHECK_OK(isqrt(result, ciphertext, key.cloud()));
  double cpu_end_time = clock();
  absl::Time end_time = absl::Now();
  std::cout << "\t\t\t\t\tTotal time: "
            << absl::ToDoubleSeconds(end_time - start_time) << " secs"
            << std::endl;
  std::cout << "\t\t\t\t\t  CPU time: "
            << (cpu_end_time - cpu_start_time) / 1'000'000 << " secs"
            << std::endl;
  return result;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: ac_int_ops_tfhe_testbench num\n\n");
    return 1;
  }
  int in;
  XLS_CHECK(absl::SimpleAtoi(argv[1], &in));
  ac_int<22, false> input(in);
  std::cout << "input: " << ac_int<22, false>(input) << std::endl;

  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  // Encode data
  auto ciphertext = Tfhe<XlsInt<22, false>>::Encrypt(input, key);
  std::cout << "Encoding done" << std::endl;

  std::cout << "Initial state check by decrypting: " << std::endl;
  // Decode results.
  std::cout << ac_int<22, false>(ciphertext.Decrypt(key)) << "\n";

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Compute the square root.
  Tfhe<XlsInt<22, false>> result = TfheSqrt(ciphertext, key);
  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  // Decode results.
  std::cout << "Decoded result: " << ac_int<22, false>(result.Decrypt(key))
            << "\n";
  std::cout << "Decoding done" << std::endl;
}
