#include "kernel_gaussian_blur.h"

#pragma hls_top
unsigned char kernel_gaussian_blur(const unsigned char window[9]) {
  // Applying the following kernel matrix:
  // [ 1   2   1]
  // [ 2   4   2]
  // [ 1   2   1]
  // and dividing result by 16
  //
  // Note: with 16 colours palette, blur effect is has limited expressiveness.
  // Also, we're trimming the value down instead of rounding to the closest
  // integer.
  return (window[0] + (window[1] << 1) + window[2] + (window[3] << 1) +
          (window[4] << 2) + (window[5] << 1) + window[6] + (window[7] << 1) +
          window[8]) >>
         4;
}
