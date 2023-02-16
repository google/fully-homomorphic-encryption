#include "transpiler/examples/memory/memory.h"

#pragma hls_top
void Memory(char state[4][4]) {
  char i, j;
#pragma hls_unroll yes
  for (i = 0; i < 4; ++i) {
#pragma hls_unroll yes
    for (j = 0; j < 4; ++j) {
      state[i][j] = data[state[i][j]];
    }
  }
}
