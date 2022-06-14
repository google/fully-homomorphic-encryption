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

#ifdef USE_YOSYS_CLEARTEXT
#include "transpiler/examples/structs/struct_with_array_yosys_cleartext.h"
#include "transpiler/examples/structs/struct_with_array_yosys_cleartext.types.h"
#else
#include "transpiler/examples/structs/struct_with_array_cleartext.h"
#include "transpiler/examples/structs/struct_with_array_cleartext.types.h"
#endif

#include "xls/common/logging/logging.h"

int main(int argc, char** argv) {
  StructWithArray input;
  input.c = 11;
  input.i.q = 44;
  input.i.c[0] = 33;
  for (int i = 1; i < C_COUNT; i++) {
    input.i.c[i] = i * 1000;
  }
  input.a[0] = 11;
  for (int i = 1; i < A_COUNT; i++) {
    input.a[i] = i * 100;
  }
  input.b[0] = -22;
  for (int i = 1; i < B_COUNT; i++) {
    input.b[i] = -i * 100;
  }
  input.z = 99;

  int other = 55;
  Inner inner;
  inner.q = 66;
  inner.c[0] = 77;
  for (int i = 1; i < C_COUNT; i++) {
    inner.c[i] = i * 1111;
  }

  Encoded<StructWithArray> encoded_struct_with_array;
  encoded_struct_with_array.Encode(input);

  Encoded<int> encoded_other;
  encoded_other.Encode(other);

  Encoded<Inner> encoded_inner;
  encoded_inner.Encode(inner);

  std::cout << "Initial round-trip check: " << std::endl << std::endl;
  StructWithArray round_trip = encoded_struct_with_array.Decode();
  std::cout << "  c: " << (signed)round_trip.c << std::endl;
  for (int i = 0; i < C_COUNT; i++) {
    std::cout << "  i.c[" << i << "]: " << round_trip.i.c[i] << std::endl;
  }
  std::cout << "  i.q: " << round_trip.i.q << std::endl;
  for (int i = 0; i < A_COUNT; i++) {
    std::cout << "  a[" << i << "]: " << round_trip.a[i] << std::endl;
  }
  for (int i = 0; i < B_COUNT; i++) {
    std::cout << "  b[" << i << "]: " << round_trip.b[i] << std::endl;
  }
  std::cout << "  z: " << (signed)round_trip.z << std::endl << std::endl;
  int other_round_trip = encoded_other.Decode();
  std::cout << "  other: " << other_round_trip << std::endl << std::endl;
  Inner inner_round_trip = encoded_inner.Decode();
  for (int i = 0; i < C_COUNT; i++) {
    std::cout << "  inner.c[" << i << "]: " << inner_round_trip.c[i]
              << std::endl;
  }
  std::cout << "  inner.q: " << inner_round_trip.q << std::endl;
  std::cout << std::endl;

  std::cout << "Starting computation." << std::endl << std::endl;
  XLS_CHECK_OK(NegateStructWithArray(encoded_struct_with_array, encoded_other,
                                     encoded_inner));

  StructWithArray result = encoded_struct_with_array.Decode();
  std::cout << "Done. Result: " << std::endl;
  std::cout << "  c: " << (signed)result.c << std::endl;
  for (int i = 0; i < C_COUNT; i++) {
    std::cout << "  i.c[" << i << "]: " << result.i.c[i] << std::endl;
  }
  std::cout << "  i.q: " << result.i.q << std::endl;
  for (int i = 0; i < A_COUNT; i++) {
    std::cout << "  a[" << i << "]: " << result.a[i] << std::endl;
  }
  for (int i = 0; i < B_COUNT; i++) {
    std::cout << "  b[" << i << "]: " << result.b[i] << std::endl;
  }
  std::cout << "  z: " << (signed)result.z << std::endl << std::endl;
  int other_result = encoded_other.Decode();
  std::cout << " other: " << other_result << std::endl << std::endl;
  Inner inner_result = encoded_inner.Decode();
  for (int i = 0; i < C_COUNT; i++) {
    std::cout << "  inner.c[" << i << "]: " << inner_result.c[i] << std::endl;
  }
  std::cout << "  inner.q: " << inner_result.q << std::endl;
  std::cout << std::endl;

  return 0;
}
