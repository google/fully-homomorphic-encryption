#include <stdio.h>

#include <array>
#include <iomanip>
#include <iostream>

#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "commons.h"
#include "transpiler/data/cleartext_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_YOSYS_INTERPRETED_CLEARTEXT
#include "transpiler/examples/image_processing/kernel_gaussian_blur_yosys_interpreted_cleartext.h"
#include "transpiler/examples/image_processing/kernel_sharpen_yosys_interpreted_cleartext.h"
#include "transpiler/examples/image_processing/ricker_wavelet_yosys_interpreted_cleartext.h"
#else
#include "transpiler/examples/image_processing/kernel_gaussian_blur_cleartext.h"
#include "transpiler/examples/image_processing/kernel_sharpen_cleartext.h"
#include "transpiler/examples/image_processing/ricker_wavelet_cleartext.h"
#endif

static void DecodeAndPrint(EncodedArrayRef<unsigned char> array) {
  absl::FixedArray<unsigned char> decoded_input = array.Decode();
  std::cout << "Decoded Input (len " << array.length() << " DIM " << MAX_PIXELS
            << std::endl;
  const int dim = MAX_PIXELS;
  for (int i = 0; i < dim; ++i) {
    for (int j = 0; j < dim; ++j) {
      std::cout << ascii_art[decoded_input[(i + 1) * (MAX_PIXELS + 2) + j + 1]];
    }
    std::cout << std::endl;
  }
}

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

  std::cout << "Running 'sharpen' filter" << std::endl;
  absl::Duration total_duration = absl::ZeroDuration();
  constexpr int total_pixels = MAX_PIXELS * MAX_PIXELS;
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      std::cout << absl::StrFormat("Processing pixel: (%d, %d) out of (%d, %d)",
                                   i + 1, j + 1, MAX_PIXELS, MAX_PIXELS);
      absl::Time t1 = absl::Now();
      double cpu_t1 = clock();
      encodedSubsetImage(encoded_input, window, i, j);
      XLS_CHECK_OK(kernel_sharpen(encoded_result[i * MAX_PIXELS + j], window));

      double last_pixel_cpu_duration = (clock() - cpu_t1) / 1'000'000;
      absl::Duration last_pixel_duration = absl::Now() - t1;
      total_duration += last_pixel_duration;

      const int pixels_processed = i * MAX_PIXELS + j + 1;
      absl::Duration remaining_time = (total_duration / pixels_processed) *
                                      (total_pixels - pixels_processed);
      std::cout << "... took " << absl::ToDoubleSeconds(last_pixel_duration)
                << " sec. (" << last_pixel_cpu_duration << " CPU sec)"
                << " ETA remaining: "
                << absl::IDivDuration(remaining_time, absl::Minutes(1),
                                      &remaining_time)
                << "m" << absl::ToInt64Seconds(remaining_time) << "s."
                << std::endl;
    }
  }
}

void runGaussianBlurFilter(EncodedArrayRef<unsigned char> encoded_input,
                           EncodedArrayRef<unsigned char> encoded_result) {
  EncodedArray<unsigned char, 9> window;

  std::cout << "Running 'Gaussian Blur' filter" << std::endl;
  absl::Duration total_duration = absl::ZeroDuration();
  constexpr int total_pixels = MAX_PIXELS * MAX_PIXELS;
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      std::cout << absl::StrFormat("Processing pixel: (%d, %d) out of (%d, %d)",
                                   i + 1, j + 1, MAX_PIXELS, MAX_PIXELS);
      absl::Time t1 = absl::Now();
      double cpu_t1 = clock();
      encodedSubsetImage(encoded_input, window, i, j);
      XLS_CHECK_OK(
          kernel_gaussian_blur(encoded_result[i * MAX_PIXELS + j], window));

      double last_pixel_cpu_duration = (clock() - cpu_t1) / 1'000'000;
      absl::Duration last_pixel_duration = absl::Now() - t1;
      total_duration += last_pixel_duration;

      const int pixels_processed = i * MAX_PIXELS + j + 1;
      absl::Duration remaining_time = (total_duration / pixels_processed) *
                                      (total_pixels - pixels_processed);
      std::cout << "... took " << absl::ToDoubleSeconds(last_pixel_duration)
                << " sec. (" << last_pixel_cpu_duration << " CPU sec)"
                << " ETA remaining: "
                << absl::IDivDuration(remaining_time, absl::Minutes(1),
                                      &remaining_time)
                << "m" << absl::ToInt64Seconds(remaining_time) << "s."
                << std::endl;
    }
  }
}

void runRickerWaveletFilter(EncodedArrayRef<unsigned char> encoded_input,
                            EncodedArrayRef<unsigned char> encoded_result) {
  EncodedArray<unsigned char, 9> window;

  std::cout << "Running 'Ricker Wavelet' filter" << std::endl;
  absl::Duration total_duration = absl::ZeroDuration();
  constexpr int total_pixels = MAX_PIXELS * MAX_PIXELS;
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      std::cout << absl::StrFormat("Processing pixel: (%d, %d) out of (%d, %d)",
                                   i + 1, j + 1, MAX_PIXELS, MAX_PIXELS);
      absl::Time t1 = absl::Now();
      double cpu_t1 = clock();
      encodedSubsetImage(encoded_input, window, i, j);
      XLS_CHECK_OK(ricker_wavelet(encoded_result[i * MAX_PIXELS + j], window));

      double last_pixel_cpu_duration = (clock() - cpu_t1) / 1'000'000;
      absl::Duration last_pixel_duration = absl::Now() - t1;
      total_duration += last_pixel_duration;

      const int pixels_processed = i * MAX_PIXELS + j + 1;
      absl::Duration remaining_time = (total_duration / pixels_processed) *
                                      (total_pixels - pixels_processed);
      std::cout << "... took " << absl::ToDoubleSeconds(last_pixel_duration)
                << " sec. (" << last_pixel_cpu_duration << " CPU sec)"
                << " ETA remaining: "
                << absl::IDivDuration(remaining_time, absl::Minutes(1),
                                      &remaining_time)
                << "m" << absl::ToInt64Seconds(remaining_time) << "s."
                << std::endl;
    }
  }
}

int main(int argc, char** argv) {
  auto padded_image =
      std::make_unique<unsigned char[]>((MAX_PIXELS + 2) * (MAX_PIXELS + 2));
  EncodedArray<unsigned char> encoded_input((MAX_PIXELS + 2) *
                                            (MAX_PIXELS + 2));
  std::cout << "Input" << std::endl;
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      padded_image[(i + 1) * (MAX_PIXELS + 2) + j + 1] =
          input_image[i * MAX_PIXELS + j];
      std::cout << ascii_art[padded_image[(i + 1) * (MAX_PIXELS + 2) + j + 1]];
    }
    std::cout << std::endl;
  }
  std::cout << "Encoding Data..." << std::endl;
  encoded_input.Encode(
      absl::MakeSpan(padded_image.get(), (MAX_PIXELS + 2) * (MAX_PIXELS + 2)));

  DecodeAndPrint(encoded_input);

  EncodedArray<unsigned char> encoded_result(MAX_PIXELS * MAX_PIXELS);
  if (chooseFilterType() == 1) {
    runSharpenFilter(encoded_input, encoded_result);
  } else if (chooseFilterType() == 2) {
    runGaussianBlurFilter(encoded_input, encoded_result);
  } else if (chooseFilterType() == 3) {
    runGaussianBlurFilter(encoded_input, encoded_result);
  }

  std::cout << "Decoding result" << std::endl;
  absl::FixedArray<unsigned char> output = encoded_result.Decode();
  std::cout << "Result:" << std::endl;
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      std::cout << ascii_art[output[i * MAX_PIXELS + j]];
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  return 0;
}
