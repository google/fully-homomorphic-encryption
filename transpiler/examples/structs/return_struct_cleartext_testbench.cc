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
#ifdef USE_YOSYS_CLEARTEXT
#include "transpiler/examples/structs/return_struct_yosys_cleartext.h"
#include "transpiler/examples/structs/return_struct_yosys_cleartext.types.h"
#else
#include "transpiler/examples/structs/return_struct_cleartext.h"
#include "transpiler/examples/structs/return_struct_cleartext.types.h"
#endif
#include "transpiler/data/cleartext_data.h"

int main(int argc, char** argv) {
  Embedded embedded;
  embedded.a = -1;
  embedded.b = 2;
  embedded.c = -4;

  Encoded<Embedded> encoded_embedded;
  encoded_embedded.Encode(embedded);

  std::cout << "Initial round-trip check: " << std::endl;
  Embedded round_trip = encoded_embedded.Decode();
  std::cout << "  A: " << static_cast<int>(round_trip.a) << std::endl;
  std::cout << "  B: " << static_cast<int>(round_trip.b) << std::endl;
  std::cout << "  C: " << static_cast<int>(round_trip.c) << std::endl;

  std::cout << "Starting computation." << std::endl;
  Encoded<ReturnStruct> encoded_result;
  Encoded<char> encoded_a;
  encoded_a.Encode(8);
  Encoded<char> encoded_c;
  encoded_c.Encode(16);
  XLS_CHECK_OK(ConstructReturnStruct(encoded_result, encoded_a,
                                     encoded_embedded, encoded_c));

  ReturnStruct result = encoded_result.Decode();
  std::cout << "Done. Result: " << std::endl;
  std::cout << " - a: " << static_cast<int>(result.a) << std::endl;
  std::cout << " - b.a: " << static_cast<int>(result.b.a) << std::endl;
  std::cout << " - b.b: " << static_cast<int>(result.b.b) << std::endl;
  std::cout << " - b.c: " << static_cast<int>(result.b.c) << std::endl;
  std::cout << " - c: " << static_cast<int>(result.c) << std::endl;
  return 0;
}
