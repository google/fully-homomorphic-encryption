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
#include <stdint.h>

#include <iostream>

#include "tfhe/tfhe.h"
#include "transpiler/data/tfhe_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_TFHE
#include "transpiler/examples/ifte/ifte_interpreted_tfhe.h"
#elif USE_YOSYS_INTERPRETED_TFHE
#include "transpiler/examples/ifte/ifte_yosys_interpreted_tfhe.h"
#else
#include "transpiler/examples/ifte/ifte_tfhe.h"
#endif

const int main_minimum_lambda = 120;

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "usage: ifte i t e" << std::endl;
    return 1;
  }

  char i = argv[1][0];
  if (i == '.') i = 0;
  char t = argv[2][0];
  char e = argv[3][0];

  // generate a keyset
  TFheGateBootstrappingParameterSet* params =
      new_default_gate_bootstrapping_parameters(main_minimum_lambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  uint32_t seed[] = {314, 1592, 657};
  tfhe_random_generator_setSeed(seed, 3);
  TFheGateBootstrappingSecretKeySet* key =
      new_random_gate_bootstrapping_secret_keyset(params);
  const TFheGateBootstrappingCloudKeySet* cloud_key = &key->cloud;

  std::cout << "i: " << i << std::endl;
  std::cout << "t: " << t << std::endl;
  std::cout << "e: " << e << std::endl;

  // Encrypt data
  auto ciphertext_i = Tfhe<bool>::Encrypt(i, key);
  auto ciphertext_t = Tfhe<char>::Encrypt(t, key);
  auto ciphertext_e = Tfhe<char>::Encrypt(e, key);

  std::cout << "Encryption done" << std::endl;

  std::cout << "Initial state check by decryption: " << std::endl;
  // Decrypt results.
  std::cout << (ciphertext_i.Decrypt(key) ? 'T' : 'F');
  std::cout << "  ";
  std::cout << static_cast<char>(ciphertext_t.Decrypt(key));
  std::cout << "  ";
  std::cout << static_cast<char>(ciphertext_e.Decrypt(key));

  std::cout << "\n";

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Perform addition
  Tfhe<char> cipher_result(params);
  XLS_CHECK_OK(
      ifte(cipher_result, ciphertext_i, ciphertext_t, ciphertext_e, cloud_key));

  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  char res = cipher_result.Decrypt(key);
  std::cout << "Decrypted result: " << res << " (hex " << std::hex << (int)res
            << ")" << std::endl;
  std::cout << "Decryption done" << std::endl;
}
