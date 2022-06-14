#include "templated_struct.h"

#pragma hls_top
StructWithArray<int, 6> CollateThem(const StructWithArray<short, 3> &a,
                                    const StructWithArray<char, 2> &b) {
  StructWithArray<int, 6> ret;
#pragma hls_unroll yes
  for (unsigned i = 0; i < 6; i++) {
    ret.data[i] = a.data[i % 3] + b.data[i % 2];
  }
  return ret;
}
