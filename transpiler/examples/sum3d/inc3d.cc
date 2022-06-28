#include "sum3d.h"

#pragma hls_top
void inc3d(int a[X_DIM][Y_DIM][Z_DIM]) {
#pragma hls_unroll yes
  for (int i = 0; i < X_DIM; i++) {
#pragma hls_unroll yes
    for (int j = 0; j < Y_DIM; j++) {
#pragma hls_unroll yes
      for (int k = 0; k < Z_DIM; k++) {
        a[i][j][k]++;
      }
    }
  }
}
