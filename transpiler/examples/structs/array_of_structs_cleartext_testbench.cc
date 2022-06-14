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
#include "transpiler/examples/structs/array_of_structs_yosys_interpreted_cleartext.h"
#include "transpiler/examples/structs/array_of_structs_yosys_interpreted_cleartext.types.h"
#else
#include "transpiler/examples/structs/array_of_structs_cleartext.h"
#include "transpiler/examples/structs/array_of_structs_cleartext.types.h"
#endif

int main(int argc, char** argv) {
  Simple simple_array[DIM_X];
  for (int i = 0; i < DIM_X; i++) {
    simple_array[i].v = i;
  }

  EncodedArray<Simple, DIM_X> bool_simple_array;
  bool_simple_array.Encode(simple_array);

  std::cout << "Initial round-trip check: " << std::endl;
  Simple round_trip[DIM_X];
  bool_simple_array.Decode(round_trip);
  for (int i = 0; i < DIM_X; i++) {
    std::cout << "  round_trip[" << i
              << "].v = " << static_cast<int>(round_trip[i].v) << std::endl;
  }

  std::cout << "Starting computation." << std::endl;
  Encoded<Simple> bool_result;
  XLS_CHECK_OK(DoubleSimpleArray(bool_result, bool_simple_array));

  Simple result = bool_result.Decode();
  std::cout << "Done." << std::endl;
  std::cout << "  v: " << std::hex << static_cast<int>(result.v) << std::endl;
  return 0;
}
