#include "struct_with_array.h"

#pragma hls_top
StructWithArray NegateStructWithArray(StructWithArray input) {
  StructWithArray result;
#pragma hls_unroll yes
  for (int i = 0; i < A_COUNT; i++) {
    result.a[i] = -input.a[i];
  }

#pragma hls_unroll yes
  for (int i = 0; i < B_COUNT; i++) {
    result.b[i] = -input.b[i];
  }
  result.c = -input.c;

  return result;
}
