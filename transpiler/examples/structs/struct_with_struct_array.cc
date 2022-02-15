#include "struct_with_struct_array.h"

#pragma hls_top
StructWithStructArray NegateStructWithStructArray(StructWithStructArray in) {
  StructWithStructArray result;
#pragma hls_unroll yes
  for (int i = 0; i < ELEMENT_COUNT; i++) {
#pragma hls_unroll yes
    for (int j = 0; j < ELEMENT_COUNT; j++) {
      result.a[i].a[j] = -in.a[i].a[j];
    }
    result.b[i] = -in.b[i];
  }

  return result;
}
