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
#include "transpiler/data/openfhe_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_OPENFHE
#include "transpiler/examples/sqrt/sqrt_interpreted_openfhe.h"
#else
#include "transpiler/examples/sqrt/sqrt_openfhe.h"
#endif

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

OpenFhe<short> FHESqrt(OpenFhe<short>& ciphertext, lbcrypto::BinFHEContext cc) {
  absl::Time start_time = absl::Now();
  double cpu_start_time = clock();
  std::cout << "Starting!" << std::endl;
  OpenFhe<short> result(cc);
  XLS_CHECK_OK(isqrt(result, ciphertext, cc));
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
    fprintf(stderr, "Usage: sqrt_tfhe_testbench num\n\n");
    return 1;
  }

  int input;
  XLS_CHECK(absl::SimpleAtoi(argv[1], &input));
  std::cout << "input: " << input << std::endl;

  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  // Encode data
  auto ciphertext = OpenFhe<short>::Encrypt(input, cc, sk);
  std::cout << "Encoding done" << std::endl;

  std::cout << "Initial state check by decrypting: " << std::endl;
  // Decode results.
  std::cout << ciphertext.Decrypt(sk) << "\n";

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Compute the square root.
  OpenFhe<short> result = FHESqrt(ciphertext, cc);
  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  // Decode results.
  std::cout << "Decoded result: " << result.Decrypt(sk) << "\n";
  std::cout << "Decoding done" << std::endl;
}
