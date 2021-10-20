#include "struct_with_array.h"

#pragma hls_top
void NegateStructWithArray(StructWithArray &input, int &other, Inner &inner) {
  input.c = -input.c;
  input.i.q = -input.i.q;
#pragma hls_unroll yes
  for (int i = 0; i < C_COUNT; i++) {
    input.i.c[i] = -input.i.c[i];
  }
#pragma hls_unroll yes
  for (int i = 0; i < A_COUNT; i++) {
    input.a[i] = -input.a[i];
  }
#pragma hls_unroll yes
  for (int i = 0; i < B_COUNT; i++) {
    input.b[i] = -input.b[i];
  }
  input.z = -input.z;
  other = -other;
  inner.q = -inner.q;
#pragma hls_unroll yes
  for (int i = 0; i < C_COUNT; i++) {
    inner.c[i] = -inner.c[i];
  }
}
