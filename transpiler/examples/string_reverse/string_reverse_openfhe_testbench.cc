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

#include <time.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "transpiler/data/openfhe_data.h"
#include "transpiler/examples/string_reverse/string_reverse.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_OPENFHE
#include "transpiler/examples/string_reverse/string_reverse_interpreted_openfhe.h"
#else
#include "transpiler/examples/string_reverse/string_reverse_openfhe.h"
#endif

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

void OpenFheStringReverse(OpenFheArray<char>& ciphertext,
                          lbcrypto::BinFHEContext cc) {
  std::cout << "Starting!" << std::endl;
  absl::Time start_time = absl::Now();
  double cpu_start_time = clock();
  XLS_CHECK_OK(ReverseString(ciphertext, cc));
  double cpu_end_time = clock();
  absl::Time end_time = absl::Now();
  std::cout << "\t\t\t\t\tTotal time: "
            << absl::ToDoubleSeconds(end_time - start_time) << " secs"
            << std::endl;
  std::cout << "\t\t\t\t\t  CPU time: "
            << (cpu_end_time - cpu_start_time) / 1'000'000 << " secs"
            << std::endl;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: string_reverse_fhe_testbench string_input"
              << std::endl;
    return 1;
  }

  std::string input = argv[1];
  input.resize(MAX_LENGTH, '\0');

  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  std::string plaintext(input);
  std::cout << "plaintext: '" << plaintext << "'" << std::endl;

  // Encrypt data
  auto ciphertext = OpenFheArray<char>::Encrypt(plaintext, cc, sk);
  std::cout << "Encryption done" << std::endl;

  std::cout << "Initial state check by decryption: " << std::endl;
  std::cout << ciphertext.Decrypt(sk) << "\n";
  std::cout << "\n";

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Perform string reverse
  OpenFheStringReverse(ciphertext, cc);
  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  std::cout << "Decrypted result: " << ciphertext.Decrypt(sk) << "\n";
  std::cout << "Decryption done" << std::endl;
}
