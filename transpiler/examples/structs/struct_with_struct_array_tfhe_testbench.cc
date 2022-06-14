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

#include "xls/common/logging/logging.h"
#ifdef USE_INTERPRETED_TFHE
#include "transpiler/examples/structs/struct_with_struct_array_interpreted_tfhe.h"
#include "transpiler/examples/structs/struct_with_struct_array_interpreted_tfhe.types.h"
#elif defined(USE_YOSYS_INTERPRETED_TFHE)
#include "transpiler/examples/structs/struct_with_struct_array_yosys_interpreted_tfhe.h"
#include "transpiler/examples/structs/struct_with_struct_array_yosys_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/structs/struct_with_struct_array_tfhe.h"
#include "transpiler/examples/structs/struct_with_struct_array_tfhe.types.h"
#endif
#include "tfhe/tfhe.h"

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

  StructWithStructArray input;
  for (int i = 0; i < ELEMENT_COUNT; i++) {
    for (int j = 0; j < ELEMENT_COUNT; j++) {
      input.a[i].a[j] = i * 64 + j;
    }
    input.b[i] = -111 * (i + 1);
  }

  Tfhe<StructWithStructArray> fhe_struct_with_struct_array(params);
  fhe_struct_with_struct_array.SetEncrypted(input, key);

  std::cout << "Round trip check: " << std::endl;
  StructWithStructArray round_trip = fhe_struct_with_struct_array.Decrypt(key);
  for (int i = 0; i < ELEMENT_COUNT; i++) {
    for (int j = 0; j < ELEMENT_COUNT; j++) {
      std::cout << "  a[" << i << "][" << j
                << "]: " << static_cast<int>(round_trip.a[i].a[j]) << std::endl;
    }
  }
  for (int i = 0; i < ELEMENT_COUNT; i++) {
    std::cout << "  b[" << i << "]: " << round_trip.b[i] << std::endl;
  }

  std::cout << "Starting computation." << std::endl;
  Tfhe<StructWithStructArray> fhe_result(params);
  XLS_CHECK_OK(NegateStructWithStructArray(
      fhe_result, fhe_struct_with_struct_array, cloud_key));

  StructWithStructArray result = fhe_result.Decrypt(key);
  std::cout << "Done. Result: " << std::endl;
  for (int i = 0; i < ELEMENT_COUNT; i++) {
    for (int j = 0; j < ELEMENT_COUNT; j++) {
      std::cout << "  a[" << i << "][" << j
                << "]: " << static_cast<int>(result.a[i].a[j]) << std::endl;
    }
  }
  for (int i = 0; i < ELEMENT_COUNT; i++) {
    std::cout << "  b[" << i << "]: " << result.b[i] << std::endl;
  }

  return 0;
}
