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
#ifdef USE_INTERPRETED_OPENFHE
#include "transpiler/examples/structs/array_of_structs_3d_interpreted_openfhe.h"
#include "transpiler/examples/structs/array_of_structs_3d_interpreted_openfhe.types.h"
#elif defined(USE_YOSYS_INTERPRETED_OPENFHE)
#include "transpiler/examples/structs/array_of_structs_3d_yosys_interpreted_openfhe.h"
#include "transpiler/examples/structs/array_of_structs_3d_yosys_interpreted_openfhe.types.h"
#else
#include "transpiler/examples/structs/array_of_structs_3d_openfhe.h"
#include "transpiler/examples/structs/array_of_structs_3d_openfhe.types.h"
#endif

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

int main(int argc, char** argv) {
  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  Simple simple_array[DIM_X][DIM_Y][DIM_Z];
  int cnt = 0;
  for (int i = 0; i < DIM_X; i++) {
    for (int j = 0; j < DIM_Y; j++) {
      for (int k = 0; k < DIM_Z; k++, cnt++) {
        simple_array[i][j][k].v = cnt;
      }
    }
  }

  OpenFheArray<Simple, DIM_X, DIM_Y, DIM_Z> openfhe_simple_array(cc);
  openfhe_simple_array.SetEncrypted(simple_array, sk);

  std::cout << "Initial round-trip check: " << std::endl;
  Simple round_trip[DIM_X][DIM_Y][DIM_Z];
  openfhe_simple_array.Decrypt(round_trip, sk);
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
  OpenFhe<Simple> tfhe_result(cc);
  XLS_CHECK_OK(DoubleSimpleArray3D(tfhe_result, openfhe_simple_array, cc));

  Simple result = tfhe_result.Decrypt(sk);
  std::cout << "Done." << std::endl;
  std::cout << "  v: " << std::hex << static_cast<int>(result.v) << std::endl;
  return 0;
}
