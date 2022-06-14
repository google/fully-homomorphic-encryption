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

#include "tfhe/tfhe.h"
#include "transpiler/examples/structs/array_of_array_of_structs_tfhe.h"
#include "transpiler/examples/structs/array_of_array_of_structs_tfhe.types.h"
#include "xls/common/logging/logging.h"

const int main_minimum_lambda = 120;

int main(int argc, char** argv) {
  // Generate a keyset.
  TFheGateBootstrappingParameterSet* params =
      new_default_gate_bootstrapping_parameters(main_minimum_lambda);

  // Generate a "random" key.
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  uint32_t seed[] = {314, 1592, 657};
  tfhe_random_generator_setSeed(seed, 3);
  TFheGateBootstrappingSecretKeySet* key =
      new_random_gate_bootstrapping_secret_keyset(params);
  const TFheGateBootstrappingCloudKeySet* cloud_key = &key->cloud;

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

  Tfhe<Base> fhe_input(params);
  fhe_input.SetEncrypted(input, key);

  std::cout << "Round trip check: " << std::endl;
  Base round_trip = fhe_input.Decrypt(key);
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
  Tfhe<Base> fhe_result(params);
  XLS_CHECK_OK(DoubleBase(fhe_result, fhe_input, cloud_key));

  Base result = fhe_result.Decrypt(key);
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
