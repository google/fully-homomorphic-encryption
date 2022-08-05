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
#include "transpiler/examples/ac_int/ac_int_ops_yosys_interpreted_openfhe.h"
#include "xls/common/logging/logging.h"

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

OpenFhe<XlsInt<22, false>> FHESqrt(OpenFhe<XlsInt<22, false>>& ciphertext,
                                   lbcrypto::BinFHEContext cc) {
  absl::Time start_time = absl::Now();
  double cpu_start_time = clock();
  std::cout << "Starting!" << std::endl;
  OpenFhe<XlsInt<22, false>> result(cc);
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
    fprintf(stderr, "Usage: ac_int_ops_openfhe_testbench num\n\n");
    return 1;
  }

  int in;
  XLS_CHECK(absl::SimpleAtoi(argv[1], &in));
  ac_int<22, false> input(in);
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
  auto ciphertext = OpenFhe<XlsInt<22, false>>::Encrypt(input, cc, sk);
  std::cout << "Encoding done" << std::endl;

  std::cout << "Initial state check by decrypting: " << std::endl;
  // Decode results.
  std::cout << ac_int<22, false>(ciphertext.Decrypt(sk)) << "\n";

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Compute the square root.
  OpenFhe<XlsInt<22, false>> result = FHESqrt(ciphertext, cc);
  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  // Decode results.
  std::cout << "Decoded result: " << ac_int<22, false>(result.Decrypt(sk))
            << "\n";
  std::cout << "Decoding done" << std::endl;
}
