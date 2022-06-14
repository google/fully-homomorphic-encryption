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
#include <cstring>
#include <iostream>

#include "transpiler/examples/structs/templated_struct.h"
#include "xls/common/logging/logging.h"
#ifdef USE_INTERPRETED_YOSYS
#include "transpiler/examples/structs/templated_struct_yosys_interpreted_cleartext.h"
#include "transpiler/examples/structs/templated_struct_yosys_interpreted_cleartext.types.h"
#else
#include "transpiler/examples/structs/templated_struct_cleartext.h"
#include "transpiler/examples/structs/templated_struct_cleartext.types.h"
#endif

int main(int argc, char** argv) {
  StructWithArray<short, 3> a = {{
      0x1234,
      0x2345,
      0x3456,
  }};
  StructWithArray<char, 2> b = {{'a', 'b'}};
  Encoded<StructWithArray<short, 3>> encoded_a(a);
  Encoded<StructWithArray<char, 2>> encoded_b(b);
  Encoded<StructWithArray<int, 6>> encoded_result;
  XLS_CHECK_OK(CollateThem(encoded_result, encoded_a, encoded_b));
  StructWithArray<int, 6> result = encoded_result.Decode();
  StructWithArray<int, 6> reference_result = CollateThem(a, b);
  if (!memcmp(&result, &reference_result, sizeof(result))) {
    std::cout << "result and reference results MATCH" << std::endl;
  } else {
    std::cout << "result and reference results DO NOT MATCH" << std::endl;
  }

  return 0;
}
