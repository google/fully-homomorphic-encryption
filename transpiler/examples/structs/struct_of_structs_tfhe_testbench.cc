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
#include "transpiler/examples/structs/struct_of_structs_interpreted_tfhe.h"
#include "transpiler/examples/structs/struct_of_structs_interpreted_tfhe.types.h"
#elif USE_YOSYS_INTERPRETED_TFHE
#include "transpiler/examples/structs/struct_of_structs_yosys_interpreted_tfhe.h"
#include "transpiler/examples/structs/struct_of_structs_yosys_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/structs/struct_of_structs_tfhe.h"
#include "transpiler/examples/structs/struct_of_structs_tfhe.types.h"
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

  StructOfStructs sos;
  sos.x = 1;
  sos.a.a = 2;
  sos.a.b = 3;
  sos.a.c = 4;
  sos.b.a = 5;
  sos.b.b = 6;
  sos.b.c = 7;
  sos.c.b.a = 8;
  sos.c.b.b = 9;
  sos.c.b.c = 10;
  sos.d.x = 11;
  sos.i.h.g.f.e.d.x = 12;

  Tfhe<StructOfStructs> fhe_sos(params);
  fhe_sos.SetEncrypted(sos, key);

  std::cout << "Initial round-trip check: " << std::endl;
  StructOfStructs round_trip = fhe_sos.Decrypt(key);
  std::cout << "  x          : " << static_cast<int>(round_trip.x) << std::endl;
  std::cout << "  a.a        : " << static_cast<int>(round_trip.a.a)
            << std::endl;
  std::cout << "  a.b        : " << static_cast<int>(round_trip.a.b)
            << std::endl;
  std::cout << "  a.c        : " << static_cast<int>(round_trip.a.c)
            << std::endl;
  std::cout << "  b.a        : " << static_cast<int>(round_trip.b.a)
            << std::endl;
  std::cout << "  b.b        : " << static_cast<int>(round_trip.b.b)
            << std::endl;
  std::cout << "  b.c        : " << static_cast<int>(round_trip.b.c)
            << std::endl;
  std::cout << "  c.b.a      : " << static_cast<int>(round_trip.c.b.a)
            << std::endl;
  std::cout << "  c.b.b      : " << static_cast<int>(round_trip.c.b.b)
            << std::endl;
  std::cout << "  c.b.c      : " << static_cast<int>(round_trip.c.b.c)
            << std::endl;
  std::cout << "  d.x        : " << static_cast<int>(round_trip.d.x)
            << std::endl;
  std::cout << "  i.h.g.f.e.d: " << static_cast<int>(round_trip.i.h.g.f.e.d.x)
            << std::endl;

  std::cout << "Starting computation." << std::endl;
  Tfhe<int> fhe_result(params);
  XLS_CHECK_OK(SumStructOfStructs(fhe_result, fhe_sos, cloud_key));

  int result = fhe_result.Decrypt(key);
  std::cout << "Done. Result: " << result << std::endl;
  return 0;
}
