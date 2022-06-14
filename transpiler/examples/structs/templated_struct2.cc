#include "templated_struct2.h"

#pragma hls_top
void convert(const Array<Tag<int>, LEN>& in,
             Tag<Array<short, LEN << 1>>& result) {
#pragma hls_unroll yes
  for (int i = 0; i < LEN; i++) {
    result.tag.data[i << 1] = short(in.data[i].tag & 0xffff);
    result.tag.data[(i << 1) + 1] = short(in.data[i].tag >> 16);
  }
}
