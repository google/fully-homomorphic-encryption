#include <stdio.h>

#include <array>
#include <iomanip>
#include <iostream>

#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "commons.h"
#include "openfhe/binfhe/binfhecontext.h"
#include "transpiler/data/openfhe_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_OPENFHE
#include "transpiler/examples/image_processing/kernel_gaussian_blur_interpreted_openfhe.h"
#include "transpiler/examples/image_processing/kernel_sharpen_interpreted_openfhe.h"
#include "transpiler/examples/image_processing/ricker_wavelet_interpreted_openfhe.h"
#elif defined(USE_YOSYS_INTERPRETED_OPENFHE)
#include "transpiler/examples/image_processing/kernel_gaussian_blur_yosys_interpreted_openfhe.h"
#include "transpiler/examples/image_processing/kernel_sharpen_yosys_interpreted_openfhe.h"
#include "transpiler/examples/image_processing/ricker_wavelet_yosys_interpreted_openfhe.h"
#else
#include "transpiler/examples/image_processing/kernel_gaussian_blur_openfhe.h"
#include "transpiler/examples/image_processing/kernel_sharpen_openfhe.h"
#include "transpiler/examples/image_processing/ricker_wavelet_openfhe.h"
#endif

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

// Copies 3x3 window from a flattened, padded, encrypted image
void encryptedSubsetImage(OpenFheArray<unsigned char>& padded_image,
                          OpenFheArray<unsigned char, 9>& window, int i, int j,
                          lbcrypto::BinFHEContext cc) {
  for (int iw = 0; iw < 3; iw++)
    for (int jw = 0; jw < 3; jw++)
      window[iw * 3 + jw] = padded_image[(i + iw) * (MAX_PIXELS + 2) + j + jw];
}

void runSharpenFilter(OpenFheArray<unsigned char>& encrypted_input,
                      OpenFheArray<unsigned char>& encrypted_result,
                      lbcrypto::BinFHEContext cc) {
  OpenFheArray<unsigned char, 9> window(cc);

  std::cout << "Running 'sharpen' filter" << std::endl;
  absl::Duration total_duration = absl::ZeroDuration();
  constexpr int total_pixels = MAX_PIXELS * MAX_PIXELS;
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      std::cout << absl::StrFormat("Processing pixel: (%d, %d) out of (%d, %d)",
                                   i + 1, j + 1, MAX_PIXELS, MAX_PIXELS);
      absl::Time t1 = absl::Now();
      double cpu_t1 = clock();
      encryptedSubsetImage(encrypted_input, window, i, j, cc);
      XLS_CHECK_OK(
          kernel_sharpen(encrypted_result[i * MAX_PIXELS + j], window, cc));

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

void runGaussianBlurFilter(OpenFheArray<unsigned char>& encrypted_input,
                           OpenFheArray<unsigned char>& encrypted_result,
                           lbcrypto::BinFHEContext cc) {
  OpenFheArray<unsigned char, 9> window(cc);

  std::cout << "Running 'Gaussian Blur' filter" << std::endl;
  absl::Duration total_duration = absl::ZeroDuration();
  constexpr int total_pixels = MAX_PIXELS * MAX_PIXELS;
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      std::cout << absl::StrFormat("Processing pixel: (%d, %d) out of (%d, %d)",
                                   i + 1, j + 1, MAX_PIXELS, MAX_PIXELS);
      absl::Time t1 = absl::Now();
      double cpu_t1 = clock();
      encryptedSubsetImage(encrypted_input, window, i, j, cc);
      XLS_CHECK_OK(kernel_gaussian_blur(encrypted_result[i * MAX_PIXELS + j],
                                        window, cc));

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

void runRickerWaveletFilter(OpenFheArray<unsigned char>& encrypted_input,
                            OpenFheArray<unsigned char>& encrypted_result,
                            lbcrypto::BinFHEContext cc) {
  OpenFheArray<unsigned char, 9> window(cc);

  std::cout << "Running 'Ricker Wavelet' filter" << std::endl;
  absl::Duration total_duration = absl::ZeroDuration();
  constexpr int total_pixels = MAX_PIXELS * MAX_PIXELS;
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      std::cout << absl::StrFormat("Processing pixel: (%d, %d) out of (%d, %d)",
                                   i + 1, j + 1, MAX_PIXELS, MAX_PIXELS);
      absl::Time t1 = absl::Now();
      double cpu_t1 = clock();
      encryptedSubsetImage(encrypted_input, window, i, j, cc);
      XLS_CHECK_OK(
          ricker_wavelet(encrypted_result[i * MAX_PIXELS + j], window, cc));

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
  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  auto padded_image =
      std::make_unique<unsigned char[]>((MAX_PIXELS + 2) * (MAX_PIXELS + 2));
  OpenFheArray<unsigned char> encrypted_input(
      (MAX_PIXELS + 2) * (MAX_PIXELS + 2), cc);
  std::cout << "Input" << std::endl;
  for (int i = 0; i < MAX_PIXELS; ++i) {
    for (int j = 0; j < MAX_PIXELS; ++j) {
      padded_image[(i + 1) * (MAX_PIXELS + 2) + j + 1] =
          input_image[i * MAX_PIXELS + j];
      std::cout << ascii_art[padded_image[(i + 1) * (MAX_PIXELS + 2) + j + 1]];
    }
    std::cout << std::endl;
  }
  std::cout << "Encrypting Data..." << std::endl;
  encrypted_input.SetEncrypted(
      absl::MakeSpan(padded_image.get(), (MAX_PIXELS + 2) * (MAX_PIXELS + 2)),
      sk);

  OpenFheArray<unsigned char> encrypted_result(MAX_PIXELS * MAX_PIXELS, cc);
  if (chooseFilterType() == 1) {
    runSharpenFilter(encrypted_input, encrypted_result, cc);
  } else if (chooseFilterType() == 2) {
    runGaussianBlurFilter(encrypted_input, encrypted_result, cc);
  } else if (chooseFilterType() == 3) {
    runRickerWaveletFilter(encrypted_input, encrypted_result, cc);
  }

  std::cout << "Decrypting result" << std::endl;
  absl::FixedArray<unsigned char> output = encrypted_result.Decrypt(sk);
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
