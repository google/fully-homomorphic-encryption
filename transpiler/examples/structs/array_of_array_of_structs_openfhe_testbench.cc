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

#include <cstdint>
#include <iostream>

#include "openfhe/binfhe/binfhecontext.h"
#include "transpiler/examples/structs/array_of_array_of_structs_openfhe.h"
#include "transpiler/examples/structs/array_of_array_of_structs_openfhe.types.h"
#include "xls/common/logging/logging.h"

const int main_minimum_lambda = 120;

int main(int argc, char** argv) {
  std::cout << "Starting key generation..." << std::endl;
  const absl::Time key_gen_start = absl::Now();

  // Generate a keyset.
  auto cc = lbcrypto::BinFHEContext();
  cc.GenerateBinFHEContext(lbcrypto::MEDIUM);

  // Generate a "random" key.
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::cout << "Generating the secret key..." << std::endl;
  auto sk = cc.KeyGen();

  std::cout << "Generating the bootstrapping keys..." << std::endl;
  cc.BTKeyGen(sk);

  std::cout << "Completed key generation ("
            << absl::ToDoubleSeconds(absl::Now() - key_gen_start) << " secs)"
            << std::endl;

  Base input;
  int input_idx = 0;
  for (int a = 0; a < A_ELEMENTS; a++) {
    for (int b = 0; b < B_ELEMENTS; b++) {
      for (int c = 0; c < C_ELEMENTS; c++) {
        for (int d = 0; d < D_ELEMENTS; d++) {
          input.a[a][b][c].a[d] = input_idx++;
        }
        input.a[a][b][c].b = input_idx++;
      }
    }
  }

  OpenFhe<Base> openfhe_input(cc);
  openfhe_input.SetEncrypted(input, sk);

  std::cout << "Round trip check: " << std::endl;
  Base round_trip = openfhe_input.Decrypt(sk);
  for (int a = 0; a < A_ELEMENTS; a++) {
    for (int b = 0; b < B_ELEMENTS; b++) {
      for (int c = 0; c < C_ELEMENTS; c++) {
        for (int d = 0; d < D_ELEMENTS; d++) {
          std::cout << "round_trip.a[" << a << "][" << b << "][" << c << "].a["
                    << d
                    << "]: " << static_cast<int>(round_trip.a[a][b][c].a[d])
                    << std::endl;
        }
        std::cout << "round_trip.a[" << a << "][" << b << "][" << c
                  << "].b: " << static_cast<int>(round_trip.a[a][b][c].b)
                  << std::endl;
      }
    }
  }

  std::cout << "Starting computation." << std::endl;
  OpenFhe<Base> openfhe_result(cc);
  XLS_CHECK_OK(DoubleBase(openfhe_result, openfhe_input, cc));

  Base result = openfhe_result.Decrypt(sk);
  std::cout << "Done. Result: " << std::endl;
  for (int a = 0; a < A_ELEMENTS; a++) {
    for (int b = 0; b < B_ELEMENTS; b++) {
      for (int c = 0; c < C_ELEMENTS; c++) {
        for (int d = 0; d < D_ELEMENTS; d++) {
          std::cout << "result.a[" << a << "][" << b << "][" << c << "].a[" << d
                    << "]: " << static_cast<int>(result.a[a][b][c].a[d])
                    << std::endl;
        }
        std::cout << "result.a[" << a << "][" << b << "][" << c
                  << "].b: " << static_cast<int>(result.a[a][b][c].b)
                  << std::endl;
      }
    }
  }

  return 0;
}
