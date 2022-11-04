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

#include "transpiler/data/tfhe_data.h"
#include "transpiler/examples/string_cap/string_cap.h"
#include "xls/common/logging/logging.h"

#if defined(USE_INTERPRETED_TFHE)
#include "transpiler/examples/string_cap/string_cap_tfhe_xls_interpreted.h"
#elif defined(USE_YOSYS_INTERPRETED_TFHE)
#include "transpiler/examples/string_cap/string_cap_tfhe_yosys_interpreted.h"
#else
#include "transpiler/examples/string_cap/string_cap_tfhe_xls_transpiled.h"
#endif

constexpr int kMainMinimumLambda = 120;

void TfheStringCap(TfheArray<char>& ciphertext,
                   const TFheGateBootstrappingCloudKeySet* bk) {
  absl::Time start_time = absl::Now();
  double cpu_start_time = clock();
  std::cout << "Starting!" << std::endl;
  XLS_CHECK_OK(CapitalizeString(ciphertext, bk));
  std::cout << "\t\t\t\t\tTotal time: "
            << absl::ToDoubleSeconds(absl::Now() - start_time) << " secs"
            << std::endl;
  std::cout << "\t\t\t\t\t  CPU time: "
            << (clock() - cpu_start_time) / 1'000'000 << " secs" << std::endl;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: string_cap_fhe_testbench string_input\n\n");
    return 1;
  }

  std::string input = argv[1];
  input.resize(MAX_LENGTH, '\0');
  // generate a keyset
  std::cout << "Starting key generation..." << std::endl;
  const absl::Time key_gen_start = absl::Now();
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 0> seed = {};
  TFHESecretKeySet key(params, seed);
  std::cout << "Completed key generation ("
            << absl::ToDoubleSeconds(absl::Now() - key_gen_start) << " secs)"
            << std::endl;

  std::string plaintext(input);
  std::cout << "plaintext: '" << plaintext << "'" << std::endl;

  std::cout << "Encrypting inputs..." << std::endl;
  const absl::Time encryption_start = absl::Now();

  // Encrypt data
  auto ciphertext = TfheArray<char>::Encrypt(plaintext, key);
  std::cout << "Encryption done (" << ciphertext.bit_width() << " bits, "
            << absl::ToDoubleSeconds(absl::Now() - encryption_start) << " secs)"
            << std::endl;

  std::cout << "Initial state check by decryption: " << std::endl;
  const absl::Time initial_decryption_start = absl::Now();
  std::cout << ciphertext.Decrypt(key) << std::endl;
  std::cout << "(took "
            << absl::ToDoubleSeconds(absl::Now() - initial_decryption_start)
            << " secs)" << std::endl;

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Perform string capitalization
  TfheStringCap(ciphertext, key.cloud());
  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  const absl::Time decryption_start = absl::Now();
  std::cout << "Decrypted result: " << ciphertext.Decrypt(key) << "\n";
  std::cout << "Decryption done ("
            << absl::ToDoubleSeconds(absl::Now() - decryption_start) << " secs)"
            << std::endl;
}
