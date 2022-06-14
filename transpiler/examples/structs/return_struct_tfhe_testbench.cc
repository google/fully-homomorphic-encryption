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
#include "transpiler/examples/structs/return_struct_interpreted_tfhe.h"
#include "transpiler/examples/structs/return_struct_interpreted_tfhe.types.h"
#elif USE_YOSYS_INTERPRETED_TFHE
#include "transpiler/examples/structs/return_struct_yosys_interpreted_tfhe.h"
#include "transpiler/examples/structs/return_struct_yosys_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/structs/return_struct_tfhe.h"
#include "transpiler/examples/structs/return_struct_tfhe.types.h"
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

  Embedded embedded;
  embedded.a = -1;
  embedded.b = 2;
  embedded.c = -4;

  Tfhe<Embedded> fhe_embedded(params);
  fhe_embedded.SetEncrypted(embedded, key);

  std::cout << "Initial round-trip check: " << std::endl;
  Embedded round_trip = fhe_embedded.Decrypt(key);
  std::cout << "  A: " << static_cast<int>(round_trip.a) << std::endl;
  std::cout << "  B: " << static_cast<int>(round_trip.b) << std::endl;
  std::cout << "  C: " << static_cast<int>(round_trip.c) << std::endl;

  std::cout << "Starting computation." << std::endl;
  Tfhe<ReturnStruct> fhe_result(params);
  auto fhe_a = Tfhe<char>::Encrypt(8, key);
  auto fhe_c = Tfhe<char>::Encrypt(16, key);
  XLS_CHECK_OK(
      ConstructReturnStruct(fhe_result, fhe_a, fhe_embedded, fhe_c, cloud_key));

  ReturnStruct result = fhe_result.Decrypt(key);
  std::cout << "Done. Result: " << std::endl;
  std::cout << " - a: " << static_cast<int>(result.a) << std::endl;
  std::cout << " - b.a: " << static_cast<int>(result.b.a) << std::endl;
  std::cout << " - b.b: " << static_cast<int>(result.b.b) << std::endl;
  std::cout << " - b.c: " << static_cast<int>(result.b.c) << std::endl;
  std::cout << " - c: " << static_cast<int>(result.c) << std::endl;
  return 0;
}
