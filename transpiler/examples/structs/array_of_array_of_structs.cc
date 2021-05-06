#include "array_of_array_of_structs.h"

#pragma hls_top
Base DoubleBase(Base input) {
  Base result;
#pragma hls_unroll yes
  for (int a = 0; a < A_ELEMENTS; a++) {
#pragma hls_unroll yes
    for (int b = 0; b < B_ELEMENTS; b++) {
#pragma hls_unroll yes
      for (int c = 0; c < C_ELEMENTS; c++) {
#pragma hls_unroll yes
        for (int d = 0; d < D_ELEMENTS; d++) {
          result.a[a][b][c].a[d] =
              input.a[a][b][c].a[d] + input.a[a][b][c].a[d];
        }
        result.a[a][b][c].b = input.a[a][b][c].b + input.a[a][b][c].b;
      }
    }
  }

  return result;
}
