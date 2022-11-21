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

#include <iostream>
#include <memory>

#include "openfhe/binfhe/binfhecontext.h"
#include "transpiler/data/openfhe_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_OPENFHE
#include "transpiler/examples/sum3d/sum3d_interpreted_openfhe.h"
#elif defined(USE_YOSYS_INTERPRETED_OPENFHE)
#include "transpiler/examples/sum3d/sum3d_yosys_interpreted_openfhe.h"
#else
#include "transpiler/examples/sum3d/sum3d_openfhe.h"
#endif

// Macros not yet propagated to generated files.
#define X_DIM 2
#define Y_DIM 3
#define Z_DIM 2

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

int main(int argc, char** argv) {
  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  // Create inputs.
  unsigned char a[X_DIM][Y_DIM][Z_DIM];
  unsigned char b[X_DIM][Y_DIM][Z_DIM];
  unsigned char input_idx = 0;
  for (int x = 0; x < X_DIM; x++) {
    for (int y = 0; y < Y_DIM; y++) {
      for (int z = 0; z < Z_DIM; z++) {
        // input_idx could roll over! Who cares?
        a[x][y][z] = input_idx++;
        b[x][y][z] = input_idx++;
      }
    }
  }

  // Encode data
  OpenFheArray<unsigned char, X_DIM, Y_DIM, Z_DIM> ciphertext_a(cc);
  OpenFheArray<unsigned char, X_DIM, Y_DIM, Z_DIM> ciphertext_b(cc);
  ciphertext_a.SetEncrypted(a, sk);
  ciphertext_b.SetEncrypted(b, sk);
  std::cout << "Encoding done" << std::endl;

  std::cout << "Initial state check: " << std::endl;
  unsigned char initial_a[X_DIM][Y_DIM][Z_DIM];
  ciphertext_a.Decrypt(initial_a, sk);
  std::cout << "A: " << std::endl;
  for (int x = 0; x < X_DIM; x++) {
    for (int y = 0; y < Y_DIM; y++) {
      for (int z = 0; z < Z_DIM; z++) {
        std::cout << "[" << x << "][" << y << "][" << z
                  << "]: " << (unsigned)initial_a[x][y][z] << std::endl;
      }
    }
  }
  std::cout << std::endl;
  std::cout << std::endl;
  unsigned char initial_b[X_DIM][Y_DIM][Z_DIM];
  ciphertext_b.Decrypt(initial_a, sk);
  std::cout << "B: " << std::endl;
  for (int x = 0; x < X_DIM; x++) {
    for (int y = 0; y < Y_DIM; y++) {
      for (int z = 0; z < Z_DIM; z++) {
        std::cout << "[" << x << "][" << y << "][" << z
                  << "]: " << (unsigned)initial_b[x][y][z] << std::endl;
      }
    }
  }
  std::cout << std::endl;

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  OpenFhe<int> cipher_result(cc);
  XLS_CHECK_OK(sum3d(cipher_result, ciphertext_a, ciphertext_b, cc));

  std::cout << "\t\t\t\t\tComputation done" << std::endl;
  std::cout << "Decoded result: ";
  std::cout << cipher_result.Decrypt(sk);
  std::cout << "\n";
  std::cout << "Decoding done" << std::endl;
}
