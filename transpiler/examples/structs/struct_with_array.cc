#include "struct_with_array.h"

#pragma hls_top
void NegateStructWithArray(StructWithArray &outer, int &other, Inner &inner) {
  outer.c = -outer.c;
  outer.i.q = -outer.i.q;
#pragma hls_unroll yes
  for (int i = 0; i < C_COUNT; i++) {
    outer.i.c[i] = -outer.i.c[i];
  }
#pragma hls_unroll yes
  for (int i = 0; i < A_COUNT; i++) {
    outer.a[i] = -outer.a[i];
  }
#pragma hls_unroll yes
  for (int i = 0; i < B_COUNT; i++) {
    outer.b[i] = -outer.b[i];
  }
  outer.z = -outer.z;
  other = -other;
  inner.q = -inner.q;
#pragma hls_unroll yes
  for (int i = 0; i < C_COUNT; i++) {
    inner.c[i] = -inner.c[i];
  }
}
