#include "transpiler/examples/image_processing/ricker_wavelet.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

TEST(RickerWaveletTest, Zeros) {
  unsigned char window[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned char expected = 0;
  unsigned char actual = ricker_wavelet(window);
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(expected, ricker_wavelet_safe_char(window));
}

TEST(RickerWaveletTest, MaxValue) {
  unsigned char window[9] = {0, 0, 0, 0, 255, 0, 0, 0, 0};
  unsigned char expected = 255;
  unsigned char actual = ricker_wavelet(window);
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(expected, ricker_wavelet_safe_char(window));
}

TEST(RickerWaveletTest, MinValueAbsReflects) {
  unsigned char window[9] = {255, 255, 255, 255, 0, 255, 255, 255, 255};
  unsigned char expected = 255;
  unsigned char actual = ricker_wavelet(window);
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(expected, ricker_wavelet_safe_char(window));
}

TEST(RickerWaveletTest, RandomValues) {
  unsigned char window[9] = {76, 35, 178, 140, 30, 205, 94, 219, 252};
  unsigned char expected = 119;
  unsigned char actual = ricker_wavelet(window);
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(expected, ricker_wavelet_safe_char(window));
}

}  // namespace
