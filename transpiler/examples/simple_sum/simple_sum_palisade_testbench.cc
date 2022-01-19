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

#include "palisade/binfhe/binfhecontext.h"
#include "palisade/binfhe/lwecore.h"
#include "transpiler/data/palisade_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_PALISADE
#include "transpiler/examples/simple_sum/simple_sum_interpreted_palisade.h"
#else
#include "transpiler/examples/simple_sum/simple_sum_palisade.h"
#endif

using namespace lbcrypto;
using namespace std;

int main(int argc, char** argv) {
  auto cc = BinFHEContext();
  cc.GenerateBinFHEContext(MEDIUM);

  std::cout << "Generating the secret key..." << std::endl;

  // Generate the secret key
  auto sk = cc.KeyGen();

  std::cout << "Generating the bootstrapping keys..." << std::endl;

  // Generate the bootstrapping keys (refresh and switching keys)
  cc.BTKeyGen(sk);

  std::cout << "Completed key generation." << std::endl;

  // create inputs.
  int x = 4052;
  int y = 913;

  cout << "inputs are " << x << " and " << y << ", sum: " << x + y << endl;
  // Encrypt data
  auto ciphertext_x = PalisadeInt::Encrypt(x, cc, sk);
  auto ciphertext_y = PalisadeInt::Encrypt(y, cc, sk);

  cout << "Encryption done" << endl;

  cout << "Initial state check by decryption: " << endl;
  // Decrypt results.
  cout << ciphertext_x.Decrypt(sk);
  cout << "  ";
  cout << ciphertext_y.Decrypt(sk);

  cout << "\n";

  cout << "\t\t\t\t\tServer side computation:" << endl;
  // Perform addition
  PalisadeInt cipher_result(cc);
  XLS_CHECK_OK(simple_sum(cipher_result.get(), ciphertext_x.get(),
                          ciphertext_y.get(), cc));

  cout << "\t\t\t\t\tComputation done" << endl;

  cout << "Decrypted result: " << cipher_result.Decrypt(sk) << "\n";

  cout << "Decryption done" << endl;
}
