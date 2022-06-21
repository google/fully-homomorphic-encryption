#include "kernel_sharpen.h"

#pragma hls_top
unsigned char kernel_sharpen(const unsigned char window[9]) {
  // Applying the following kernel matrix:
  // [ 0  -1   0]
  // [-1   5  -1]
  // [ 0  -1   0]
  unsigned char value =
      ((window[4] * 5) - window[5] - window[7] - window[1] - window[3]);
  if (value > 75) {
    // Value can't be more than 15*5 = 75
    // Overflow = negative values, clamp to zero
    return 0;
  }
  if (value > 15) {
    // Positive value, but larger than palette maximum
    // Clamp to 15
    return 15;
  }
  return value;
}
