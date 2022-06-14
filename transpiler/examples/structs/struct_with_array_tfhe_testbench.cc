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
#include "transpiler/examples/structs/struct_with_array_interpreted_tfhe.h"
#include "transpiler/examples/structs/struct_with_array_interpreted_tfhe.types.h"
#elif USE_YOSYS_INTERPRETED_TFHE
#include "transpiler/examples/structs/struct_with_array_yosys_interpreted_tfhe.h"
#include "transpiler/examples/structs/struct_with_array_yosys_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/structs/struct_with_array_tfhe.h"
#include "transpiler/examples/structs/struct_with_array_tfhe.types.h"
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

  StructWithArray input;
  input.c = 11;
  input.i.q = 44;
  input.i.c[0] = 33;
  for (int i = 1; i < C_COUNT; i++) {
    input.i.c[i] = i * 1000;
  }
  input.a[0] = 11;
  for (int i = 1; i < A_COUNT; i++) {
    input.a[i] = i * 100;
  }
  input.b[0] = -22;
  for (int i = 1; i < B_COUNT; i++) {
    input.b[i] = -i * 100;
  }
  input.z = 99;

  int other = 55;
  Inner inner;
  inner.q = 66;
  inner.c[0] = 77;
  for (int i = 1; i < C_COUNT; i++) {
    inner.c[i] = i * 1111;
  }

  Tfhe<StructWithArray> fhe_struct_with_array(params);
  fhe_struct_with_array.SetEncrypted(input, key);

  Tfhe<int> fhe_other(params);
  fhe_other.SetEncrypted(other, key);

  Tfhe<Inner> fhe_inner(params);
  fhe_inner.SetEncrypted(inner, key);

  std::cout << "Initial round-trip check: " << std::endl << std::endl;
  StructWithArray round_trip = fhe_struct_with_array.Decrypt(key);
  std::cout << "  c: " << (signed)round_trip.c << std::endl;
  for (int i = 0; i < C_COUNT; i++) {
    std::cout << "  i.c[" << i << "]: " << round_trip.i.c[i] << std::endl;
  }
  std::cout << "  i.q: " << round_trip.i.q << std::endl;
  for (int i = 0; i < A_COUNT; i++) {
    std::cout << "  a[" << i << "]: " << round_trip.a[i] << std::endl;
  }
  for (int i = 0; i < B_COUNT; i++) {
    std::cout << "  b[" << i << "]: " << round_trip.b[i] << std::endl;
  }
  std::cout << "  z: " << (signed)round_trip.z << std::endl << std::endl;
  int other_round_trip = fhe_other.Decrypt(key);
  std::cout << "  other: " << other_round_trip << std::endl << std::endl;
  Inner inner_round_trip = fhe_inner.Decrypt(key);
  for (int i = 0; i < C_COUNT; i++) {
    std::cout << "  inner.c[" << i << "]: " << inner_round_trip.c[i]
              << std::endl;
  }
  std::cout << "  inner.q: " << inner_round_trip.q << std::endl;
  std::cout << std::endl;

  std::cout << "Starting computation." << std::endl << std::endl;
  XLS_CHECK_OK(NegateStructWithArray(fhe_struct_with_array, fhe_other,
                                     fhe_inner, cloud_key));

  StructWithArray result = fhe_struct_with_array.Decrypt(key);
  std::cout << "Done. Result: " << std::endl;
  std::cout << "  c: " << (signed)result.c << std::endl;
  for (int i = 0; i < C_COUNT; i++) {
    std::cout << "  i.c[" << i << "]: " << result.i.c[i] << std::endl;
  }
  std::cout << "  i.q: " << result.i.q << std::endl;
  for (int i = 0; i < A_COUNT; i++) {
    std::cout << "  a[" << i << "]: " << result.a[i] << std::endl;
  }
  for (int i = 0; i < B_COUNT; i++) {
    std::cout << "  b[" << i << "]: " << result.b[i] << std::endl;
  }
  std::cout << "  z: " << (signed)result.z << std::endl << std::endl;
  int other_result = fhe_other.Decrypt(key);
  std::cout << " other: " << other_result << std::endl << std::endl;
  Inner inner_result = fhe_inner.Decrypt(key);
  for (int i = 0; i < C_COUNT; i++) {
    std::cout << "  inner.c[" << i << "]: " << inner_result.c[i] << std::endl;
  }
  std::cout << "  inner.q: " << inner_result.q << std::endl;
  std::cout << std::endl;

  return 0;
}
