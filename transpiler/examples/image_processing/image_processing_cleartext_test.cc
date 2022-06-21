#include <stdio.h>

#include <array>
#include <iomanip>
#include <iostream>

#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "commons.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "kernel_gaussian_blur.h"
#include "kernel_sharpen.h"
#include "transpiler/data/cleartext_data.h"
#include "transpiler/examples/image_processing/kernel_gaussian_blur_yosys_interpreted_cleartext.h"
#include "transpiler/examples/image_processing/kernel_sharpen_yosys_interpreted_cleartext.h"
#include "xls/common/logging/logging.h"

// Copies 3x3 window from a flattened, padded, encoded image
void encodedSubsetImage(EncodedArrayRef<unsigned char> padded_image,
                        EncodedArrayRef<unsigned char, 9> window, int i,
                        int j) {
  for (int iw = -1; iw < 2; iw++)
    for (int jw = -1; jw < 2; jw++) {
      window[(iw + 1) * 3 + jw + 1] =
          padded_image[(i + iw + 1) * (MAX_PIXELS + 2) + j + jw + 1];
    }
}

void runSharpenFilter(EncodedArrayRef<unsigned char> encoded_input,
                      EncodedArrayRef<unsigned char> encoded_result) {
  EncodedArray<unsigned char, 9> window;

  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      encodedSubsetImage(encoded_input, window, i, j);
      XLS_CHECK_OK(kernel_sharpen(encoded_result[i * MAX_PIXELS + j], window));
    }
  }
}

void runGaussianBlurFilter(EncodedArrayRef<unsigned char> encoded_input,
                           EncodedArrayRef<unsigned char> encoded_result) {
  EncodedArray<unsigned char, 9> window;

  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      encodedSubsetImage(encoded_input, window, i, j);
      XLS_CHECK_OK(
          kernel_gaussian_blur(encoded_result[i * MAX_PIXELS + j], window));
    }
  }
}

TEST(ImageProcessingTest, Sharpen) {
  auto padded_image =
      std::make_unique<unsigned char[]>((MAX_PIXELS + 2) * (MAX_PIXELS + 2));
  EncodedArray<unsigned char> encoded_input((MAX_PIXELS + 2) *
                                            (MAX_PIXELS + 2));
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      padded_image[(i + 1) * (MAX_PIXELS + 2) + j + 1] =
          input_image[i * MAX_PIXELS + j];
    }
  }
  encoded_input.Encode(
      absl::MakeSpan(padded_image.get(), (MAX_PIXELS + 2) * (MAX_PIXELS + 2)));

  EncodedArray<unsigned char> encoded_result(MAX_PIXELS * MAX_PIXELS);

  runSharpenFilter(encoded_input, encoded_result);

  absl::FixedArray<unsigned char> decoded_output = encoded_result.Decode();

  unsigned char cleartext_output[MAX_PIXELS * MAX_PIXELS] = {0};

  // Apply filter (no encryption, plaintext)
  unsigned char window[9];
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      subsetImage(padded_image.get(), window, i, j);
      cleartext_output[i * MAX_PIXELS + j] = kernel_sharpen(window);
      XLS_CHECK_EQ(cleartext_output[i * MAX_PIXELS + j],
                   decoded_output[i * MAX_PIXELS + j]);
    }
  }
}

TEST(ImageProcessingTest, GaussianBlur) {
  auto padded_image =
      std::make_unique<unsigned char[]>((MAX_PIXELS + 2) * (MAX_PIXELS + 2));
  EncodedArray<unsigned char> encoded_input((MAX_PIXELS + 2) *
                                            (MAX_PIXELS + 2));
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      padded_image[(i + 1) * (MAX_PIXELS + 2) + j + 1] =
          input_image[i * MAX_PIXELS + j];
    }
  }
  encoded_input.Encode(
      absl::MakeSpan(padded_image.get(), (MAX_PIXELS + 2) * (MAX_PIXELS + 2)));

  EncodedArray<unsigned char> encoded_result(MAX_PIXELS * MAX_PIXELS);

  runGaussianBlurFilter(encoded_input, encoded_result);

  absl::FixedArray<unsigned char> decoded_output = encoded_result.Decode();

  unsigned char cleartext_output[MAX_PIXELS * MAX_PIXELS] = {0};

  // Apply filter (no encryption, plaintext)
  unsigned char window[9];
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      subsetImage(padded_image.get(), window, i, j);
      cleartext_output[i * MAX_PIXELS + j] = kernel_gaussian_blur(window);
      XLS_CHECK_EQ(cleartext_output[i * MAX_PIXELS + j],
                   decoded_output[i * MAX_PIXELS + j]);
    }
  }
}
