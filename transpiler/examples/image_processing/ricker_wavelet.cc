#include "ricker_wavelet.h"

#pragma hls_top
unsigned char ricker_wavelet(const unsigned char window[9]) {
  // Applies the kernel:
  //
  //  [[ -1,   -1,   -1],
  //   [ -1,    8,   -1],
  //   [ -1,   -1,   -1]]
  //
  // Max and min values are +/-2040, which fits in an int16_t/short,
  short avg_neighbors = (window[0] + window[1] + window[2] + window[3] +
                         window[5] + window[6] + window[7] + window[8]) /
                        8;

  short diff = window[4] - avg_neighbors;
  unsigned short abs_diff = diff > 0 ? diff : -diff;
  return (unsigned char)abs_diff;
}

unsigned char safe_mean(const unsigned char a, const unsigned char b) {
  return (a & b) + ((a ^ b) >> 1);
}

// A version that does not overflow unsigned char, though with yosys
// optimization it still produces a circuit with ~50 more gates than the naive
// version.
unsigned char ricker_wavelet_safe_char(const unsigned char window[9]) {
  // Creating a Ricker Wavelet by recursive overflow safe mean
  unsigned char avg = safe_mean(safe_mean(safe_mean(window[0], window[1]),
                                          safe_mean(window[2], window[3])),
                                safe_mean(safe_mean(window[5], window[6]),
                                          safe_mean(window[7], window[8])));

  if (window[4] > avg)
    return window[4] - avg;
  else
    return avg - window[4];
}
