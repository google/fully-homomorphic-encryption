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
#include "transpiler/examples/structs/simple_struct_tfhe.h"
#include "transpiler/examples/structs/simple_struct_tfhe.types.h"
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

  SimpleStruct simple_struct;
  simple_struct.a = 2;
  simple_struct.b = -4;
  simple_struct.c = 8;

  Tfhe<SimpleStruct> fhe_simple_struct(params);
  fhe_simple_struct.SetEncrypted(simple_struct, key);

  std::cout << "Initial round-trip check: " << std::endl;
  SimpleStruct round_trip = fhe_simple_struct.Decrypt(key);
  std::cout << "  A: " << static_cast<int>(round_trip.a) << std::endl;
  std::cout << "  B: " << static_cast<int>(round_trip.b) << std::endl;
  std::cout << "  C: " << static_cast<int>(round_trip.c) << std::endl
            << std::endl;

  std::cout << "Starting computation." << std::endl;
  Tfhe<int> fhe_result(params);
  XLS_CHECK_OK(SumSimpleStruct(fhe_result, fhe_simple_struct, cloud_key));

  int result = fhe_result.Decrypt(key);
  std::cout << "Done. Result: " << result << std::endl;
  return 0;
}
