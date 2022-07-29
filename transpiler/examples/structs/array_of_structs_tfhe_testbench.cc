// Copyright 2022 Google LLC
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

#include "transpiler/examples/structs/array_of_structs.h"
#include "xls/common/logging/logging.h"
#ifdef USE_INTERPRETED_YOSYS
#include "transpiler/examples/structs/array_of_structs_yosys_interpreted_tfhe.h"
#include "transpiler/examples/structs/array_of_structs_yosys_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/structs/array_of_structs_tfhe.h"
#include "transpiler/examples/structs/array_of_structs_tfhe.types.h"
#endif

constexpr int kMainMinimumLambda = 120;

// Random seed for key generation
// Note: In real applications, a cryptographically secure seed needs to be used.
constexpr std::array<uint32_t, 3> kSeed = {314, 1592, 657};

int main(int argc, char** argv) {
  TFHEParameters params(kMainMinimumLambda);
  TFHESecretKeySet key(params, kSeed);

  Simple simple_array[DIM_X];
  for (int i = 0; i < DIM_X; i++) {
    simple_array[i].v = i;
  }

  TfheArray<Simple, DIM_X> tfhe_simple_array =
      TfheArray<Simple, DIM_X>::Encrypt(simple_array, key);

  std::cout << "Initial round-trip check: " << std::endl;
  Simple round_trip[DIM_X];
  tfhe_simple_array.Decrypt(round_trip, key);
  for (int i = 0; i < DIM_X; i++) {
    std::cout << "  round_trip[" << i
              << "].v = " << static_cast<int>(round_trip[i].v) << std::endl;
  }

  std::cout << "Starting computation." << std::endl;
  Tfhe<Simple> tfhe_result(params);
  XLS_CHECK_OK(DoubleSimpleArray(tfhe_result, tfhe_simple_array, key.cloud()));

  Simple result = tfhe_result.Decrypt(key);
  std::cout << "Done." << std::endl;
  std::cout << "  v: " << std::hex << static_cast<int>(result.v) << std::endl;
  return 0;
}
