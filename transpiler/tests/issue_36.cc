#include "transpiler/tests/issue_36.h"

#pragma hls_top
void sort(int list[10], char L, char R) {
  char i, j;
  int pv;
  int tmp;

  i = L;
  j = R;

  pv = list[(L + R) / 2];

#pragma hls_unroll yes
  for (char k = 0; k < 4; k++) {
#pragma hls_unroll yes
    for (char ki = 0; ki < 4; ki++) {
      if (list[i] < pv) break;
      i++;
    }

#pragma hls_unroll yes
    for (char kj = 0; kj < 4; kj++) {
      if (pv < list[j]) break;
      j--;
    }

    if (i >= j) break;

    tmp = list[i];
    list[i] = list[j];
    list[j] = tmp;

    i++;
    j--;
  }
}
