#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_IMAGE_PROCESSING_RICKER_WAVELET_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_IMAGE_PROCESSING_RICKER_WAVELET_H_

// Computes a Ricker Wavelet of size 3x3
unsigned char ricker_wavelet(const unsigned char window[9]);

// A version of ricker_wavelet that stays within unsigned char without overflow
unsigned char ricker_wavelet_safe_char(const unsigned char window[9]);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_IMAGE_PROCESSING_RICKER_WAVELET_H_
