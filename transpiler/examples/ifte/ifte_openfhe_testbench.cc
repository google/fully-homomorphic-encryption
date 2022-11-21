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

#include "openfhe/binfhe/binfhecontext.h"
#include "transpiler/data/openfhe_data.h"
#include "xls/common/logging/logging.h"

#if defined(USE_INTERPRETED_OPENFHE)
#include "transpiler/examples/ifte/ifte_interpreted_openfhe.h"
#elif defined(USE_YOSYS_INTERPRETED_OPENFHE)
#include "transpiler/examples/ifte/ifte_yosys_interpreted_openfhe.h"
#else
#include "transpiler/examples/ifte/ifte_openfhe.h"
#endif

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

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
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  std::cout << "i: " << i << std::endl;
  std::cout << "t: " << t << std::endl;
  std::cout << "e: " << e << std::endl;

  // Encrypt data
  auto ciphertext_i = OpenFhe<bool>::Encrypt(i, cc, sk);
  auto ciphertext_t = OpenFhe<char>::Encrypt(t, cc, sk);
  auto ciphertext_e = OpenFhe<char>::Encrypt(e, cc, sk);

  std::cout << "Encryption done" << std::endl;

  std::cout << "Initial state check by decryption: " << std::endl;
  // Decrypt results.
  std::cout << (ciphertext_i.Decrypt(sk) ? 'T' : 'F');
  std::cout << "  ";
  std::cout << static_cast<char>(ciphertext_t.Decrypt(sk));
  std::cout << "  ";
  std::cout << static_cast<char>(ciphertext_e.Decrypt(sk));

  std::cout << "\n";

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Perform addition
  OpenFhe<char> cipher_result(cc);
  XLS_CHECK_OK(
      ifte(cipher_result, ciphertext_i, ciphertext_t, ciphertext_e, cc));

  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  char res = cipher_result.Decrypt(sk);
  std::cout << "Decrypted result: " << res << " (hex " << std::hex << (int)res
            << ")" << std::endl;
  std::cout << "Decryption done" << std::endl;
}
