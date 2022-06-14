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
#include "transpiler/examples/structs/array_of_structs_3d_yosys_interpreted_tfhe.h"
#include "transpiler/examples/structs/array_of_structs_3d_yosys_interpreted_tfhe.types.h"
#elif USE_INTERPRETED_TFHE
#include "transpiler/examples/structs/array_of_structs_3d_interpreted_tfhe.h"
#include "transpiler/examples/structs/array_of_structs_3d_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/structs/array_of_structs_3d_tfhe.h"
#include "transpiler/examples/structs/array_of_structs_3d_tfhe.types.h"
#endif

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

  Simple simple_array[DIM_X][DIM_Y][DIM_Z];
  int cnt = 0;
  for (int i = 0; i < DIM_X; i++) {
    for (int j = 0; j < DIM_Y; j++) {
      for (int k = 0; k < DIM_Z; k++, cnt++) {
        simple_array[i][j][k].v = cnt;
      }
    }
  }

  TfheArray<Simple, DIM_X, DIM_Y, DIM_Z> tfhe_simple_array(params);
  tfhe_simple_array.SetEncrypted(simple_array, key);

  std::cout << "Initial round-trip check: " << std::endl;
  Simple round_trip[DIM_X][DIM_Y][DIM_Z];
  tfhe_simple_array.Decrypt(round_trip, key);
  for (int i = 0; i < DIM_X; i++) {
    for (int j = 0; j < DIM_Y; j++) {
      for (int k = 0; k < DIM_Z; k++) {
        std::cout << "  round_trip[" << i << "][" << j << "][" << k
                  << "].v = " << static_cast<int>(round_trip[i][j][k].v)
                  << std::endl;
      }
    }
  }

  std::cout << "Starting computation." << std::endl;
  Tfhe<Simple> tfhe_result(params);
  XLS_CHECK_OK(DoubleSimpleArray3D(tfhe_result, tfhe_simple_array, cloud_key));

  Simple result = tfhe_result.Decrypt(key);
  std::cout << "Done." << std::endl;
  std::cout << "  v: " << std::hex << static_cast<int>(result.v) << std::endl;
  return 0;
}
