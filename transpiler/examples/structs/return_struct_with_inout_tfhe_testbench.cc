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
#ifdef USE_YOSYS_INTERPRETED_TFHE
#include "transpiler/examples/structs/return_struct_with_inout_yosys_interpreted_tfhe.h"
#include "transpiler/examples/structs/return_struct_with_inout_yosys_interpreted_tfhe.types.h"
#elif defined(DUSE_INTERPRETED_TFHE)
#include "transpiler/examples/structs/return_struct_with_inout_interpreted_tfhe.h"
#include "transpiler/examples/structs/return_struct_with_inout_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/structs/return_struct_with_inout_tfhe.h"
#include "transpiler/examples/structs/return_struct_with_inout_tfhe.types.h"
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

  Helper helper_a;
  helper_a.a = -1;
  helper_a.b = 0xffffffff;
  helper_a.c = -4;

  Helper helper_b;
  helper_b.a = 1;
  helper_b.b = 0x7fffffff;
  helper_b.c = 32;

  Helper helper_c;
  helper_c.a = -64;
  helper_c.b = 128;
  helper_c.c = -256;

  Tfhe<Helper> fhe_helper_a(params);
  fhe_helper_a.SetEncrypted(helper_a, key);
  Tfhe<Helper> fhe_helper_b(params);
  fhe_helper_b.SetEncrypted(helper_b, key);
  Tfhe<Helper> fhe_helper_c(params);
  fhe_helper_c.SetEncrypted(helper_c, key);

  std::cout << "Round trip check : helper_a: " << std::endl;
  Helper round_trip_a = fhe_helper_a.Decrypt(key);
  std::cout << "  a: " << static_cast<int>(round_trip_a.a) << std::endl;
  std::cout << "  b: " << static_cast<unsigned int>(round_trip_a.b)
            << std::endl;
  std::cout << "  c: " << static_cast<int>(round_trip_a.c) << std::endl;

  std::cout << "Round trip check : helper_b: " << std::endl;
  Helper round_trip_b = fhe_helper_b.Decrypt(key);
  std::cout << "  a: " << static_cast<int>(round_trip_b.a) << std::endl;
  std::cout << "  b: " << static_cast<unsigned int>(round_trip_b.b)
            << std::endl;
  std::cout << "  c: " << static_cast<int>(round_trip_b.c) << std::endl;

  std::cout << "Round trip check : helper_c: " << std::endl;
  Helper round_trip_c = fhe_helper_c.Decrypt(key);
  std::cout << "  a: " << static_cast<int>(round_trip_c.a) << std::endl;
  std::cout << "  b: " << static_cast<unsigned int>(round_trip_c.b)
            << std::endl;
  std::cout << "  c: " << static_cast<int>(round_trip_c.c) << std::endl;

  std::cout << "Starting computation." << std::endl;
  Tfhe<ReturnStruct> fhe_result(params);
  XLS_CHECK_OK(ConstructReturnStructWithInout(
      fhe_result, fhe_helper_a, fhe_helper_b, fhe_helper_c, cloud_key));

  ReturnStruct result = fhe_result.Decrypt(key);
  std::cout << "Done. Function return: " << std::endl;
  std::cout << " - a: " << static_cast<int>(result.a) << std::endl;
  std::cout << " - b.a: " << static_cast<int>(result.b.a) << std::endl;
  std::cout << " - b.b: " << static_cast<unsigned int>(result.b.b) << std::endl;
  std::cout << " - b.c: " << static_cast<int>(result.b.c) << std::endl;
  std::cout << " - c: " << static_cast<int>(result.c) << std::endl << std::endl;

  std::cout << "helper_a return-by-reference: " << std::endl;
  Helper output_a = fhe_helper_a.Decrypt(key);
  std::cout << "  a: " << static_cast<int>(output_a.a) << std::endl;
  std::cout << "  b: " << static_cast<unsigned int>(output_a.b) << std::endl;
  std::cout << "  c: " << static_cast<int>(output_a.c) << std::endl
            << std::endl;

  std::cout << "helper_b return-by-reference: " << std::endl;
  Helper output_b = fhe_helper_b.Decrypt(key);
  std::cout << "  a: " << static_cast<int>(output_b.a) << std::endl;
  std::cout << "  b: " << static_cast<unsigned int>(output_b.b) << std::endl;
  std::cout << "  c: " << static_cast<int>(output_b.c) << std::endl
            << std::endl;

  std::cout << "helper_c (should be unchanged!): " << std::endl;
  Helper output_c = fhe_helper_c.Decrypt(key);
  std::cout << "  a: " << static_cast<int>(output_c.a) << std::endl;
  std::cout << "  b: " << static_cast<unsigned int>(output_c.b) << std::endl;
  std::cout << "  c: " << static_cast<int>(output_c.c) << std::endl
            << std::endl;
  return 0;
}
