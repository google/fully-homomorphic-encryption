#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_IMAGE_PROCESSING_KERNEL_SHARPEN_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_IMAGE_PROCESSING_KERNEL_SHARPEN_H_

// Computes "sharpen" filter using kernel of size 3x3
// Inputs and the output are greyscale colors in 0..15 range.
// If the result is larger than 15, it should be clamped to zero by the client.
unsigned char kernel_sharpen(const unsigned char window[9]);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_IMAGE_PROCESSING_KERNEL_SHARPEN_H_
