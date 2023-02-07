#include <stdio.h>

#include <iomanip>
#include <iostream>

#include "commons.h"
#include "kernel_gaussian_blur.h"
#include "kernel_sharpen.h"
#include "ricker_wavelet.h"
using namespace std;

int main(int argc, char** argv) {
  // Prepare and print the input
  unsigned char padded_image[(MAX_PIXELS + 2) * (MAX_PIXELS + 2)] = {0};
  cout << "Input image:" << endl;
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      padded_image[(i + 1) * (MAX_PIXELS + 2) + j + 1] =
          input_image[i * MAX_PIXELS + j];
      cout << ascii_art[padded_image[(i + 1) * (MAX_PIXELS + 2) + j + 1]];
    }
    cout << endl;
  }
  unsigned char output[MAX_PIXELS * MAX_PIXELS] = {0};

  // Apply filter (no encryption, plaintext)
  unsigned char window[9];
  int filterType = chooseFilterType();
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      subsetImage(padded_image, window, i, j);
      output[i * MAX_PIXELS + j] = filterType == 1 ? kernel_sharpen(window)
                                   : filterType == 2
                                       ? kernel_gaussian_blur(window)
                                       : ricker_wavelet(window);
    }
  }

  // Print results
  cout << "Result:" << endl;
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      cout << ascii_art[output[i * MAX_PIXELS + j]];
    }
    cout << endl;
  }
  fprintf(stderr, "\r\n");
  return 0;
}
