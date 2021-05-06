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
#include "transpiler/examples/structs/struct_with_array_tfhe.h"
#include "transpiler/examples/structs/struct_with_array_tfhe.types.h"
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

  StructWithArray input;
  for (int i = 0; i < A_COUNT; i++) {
    input.a[i] = i * 100;
  }
  for (int i = 0; i < B_COUNT; i++) {
    input.b[i] = -i * 100;
  }
  input.c = 1024;

  FheStructWithArray fhe_struct_with_array(params);
  fhe_struct_with_array.SetEncrypted(input, key);

  std::cout << "Initial round-trip check: " << std::endl;
  StructWithArray round_trip = fhe_struct_with_array.Decrypt(key);
  for (int i = 0; i < A_COUNT; i++) {
    std::cout << "  a[" << i << "]: " << round_trip.a[i] << std::endl;
  }
  for (int i = 0; i < B_COUNT; i++) {
    std::cout << "  b[" << i << "]: " << round_trip.b[i] << std::endl;
  }
  std::cout << "  c: " << round_trip.c << std::endl << std::endl;

  std::cout << "Starting computation." << std::endl;
  FheStructWithArray fhe_result(params);
  XLS_CHECK_OK(NegateStructWithArray(fhe_result.get(),
                                     fhe_struct_with_array.get(), cloud_key));

  StructWithArray result = fhe_result.Decrypt(key);
  std::cout << "Done. Result: " << std::endl;
  for (int i = 0; i < A_COUNT; i++) {
    std::cout << "  a[" << i << "]: " << result.a[i] << std::endl;
  }
  for (int i = 0; i < B_COUNT; i++) {
    std::cout << "  b[" << i << "]: " << result.b[i] << std::endl;
  }
  std::cout << "  c: " << result.c << std::endl << std::endl;

  return 0;
}
