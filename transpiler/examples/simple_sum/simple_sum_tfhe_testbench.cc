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

#if defined(USE_INTERPRETED_TFHE)
#include "transpiler/examples/simple_sum/simple_sum_interpreted_tfhe.h"
#elif defined(USE_YOSYS_INTERPRETED_TFHE)
#include "transpiler/examples/simple_sum/simple_sum_yosys_interpreted_tfhe.h"
#else
#include "transpiler/examples/simple_sum/simple_sum_tfhe.h"
#endif

using namespace std;

const int main_minimum_lambda = 120;

int main(int argc, char** argv) {
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

  // create inputs.
  int x = 4052;
  int y = 913;

  cout << "inputs are " << x << " and " << y << ", sum: " << x + y << endl;
  // Encrypt data
  auto ciphertext_x = Tfhe<int>::Encrypt(x, key);
  auto ciphertext_y = Tfhe<int>::Encrypt(y, key);

  cout << "Encryption done" << endl;

  cout << "Initial state check by decryption: " << endl;
  // Decrypt results.
  cout << ciphertext_x.Decrypt(key);
  cout << "  ";
  cout << ciphertext_y.Decrypt(key);

  cout << "\n";

  cout << "\t\t\t\t\tServer side computation:" << endl;
  // Perform addition
  Tfhe<int> cipher_result(params);
  XLS_CHECK_OK(
      simple_sum(cipher_result, ciphertext_x, ciphertext_y, cloud_key));

  cout << "\t\t\t\t\tComputation done" << endl;

  cout << "Decrypted x: " << ciphertext_x.Decrypt(key) << "\n";
  cout << "Decrypted y: " << ciphertext_y.Decrypt(key) << "\n";
  cout << "Decrypted result: " << cipher_result.Decrypt(key) << "\n";

  cout << "Decryption done" << endl;
}
