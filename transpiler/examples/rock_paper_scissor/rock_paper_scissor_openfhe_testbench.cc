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

#ifdef USE_INTERPRETED_OPENFHE
#include "transpiler/examples/rock_paper_scissor/rock_paper_scissor_interpreted_openfhe.h"
#else
#include "transpiler/examples/rock_paper_scissor/rock_paper_scissor_openfhe.h"
#endif

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

bool is_valid_selection(char c) {
  if ((c == 'R') || (c == 'P') || (c == 'S')) {
    return true;
  }
  return false;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "usage: rock_paper_scissor [R,P,S] [R,P,S]" << std::endl;
    return 1;
  }

  if (!is_valid_selection(argv[1][0]) || !is_valid_selection(argv[2][0])) {
    std::cerr << "usage: rock_paper_scissor [R,P,S] [R,P,S]" << std::endl;
    return 1;
  }

  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  // create inputs.
  char player_a = argv[1][0];
  char player_b = argv[2][0];

  std::cout << "Player A selected " << player_a << " and Player B selected "
            << player_b << std::endl;
  // Encrypt data
  auto ciphertext_x = OpenFhe<char>::Encrypt(player_a, cc, sk);
  auto ciphertext_y = OpenFhe<char>::Encrypt(player_b, cc, sk);

  std::cout << "Encryption done" << std::endl;

  std::cout << "Initial state check by decryption: " << std::endl;
  // Decrypt results.
  std::cout << static_cast<char>(ciphertext_x.Decrypt(sk));
  std::cout << "  ";
  std::cout << static_cast<char>(ciphertext_y.Decrypt(sk));

  std::cout << "\n";

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Perform addition
  OpenFhe<char> cipher_result(cc);
  XLS_CHECK_OK(
      rock_paper_scissor(cipher_result, ciphertext_x, ciphertext_y, cc));

  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  std::cout << "Decrypted result: " << (char)cipher_result.Decrypt(sk) << "\n";
  std::cout << "Decryption done" << std::endl;
}
