#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_IMAGE_PROCESSING_COMMONS_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_IMAGE_PROCESSING_COMMONS_H_

// The demo processes [8x8] images.
// That's tiny, but works as a proof of concept.
#define MAX_PIXELS 8

// Extracts a 3x3 window around the given pixel
void subsetImage(unsigned char input[(MAX_PIXELS + 2) * (MAX_PIXELS + 2)],
                 unsigned char window[9], int i, int j);

// Prompts user to enter the filter type
// Returns 1 for "Sharpen filter"
// Returns 2 for "Gaussian Blur"
int chooseFilterType();

// A sample hard-coded image that includes two disconnected blocks.
// Inputs using a 4-bit grayscale palette to fit in chars and accomodate for
// overflows.
const unsigned char input_image[MAX_PIXELS * MAX_PIXELS] = {
    00, 00, 00, 00, 00, 00, 00, 00,  // Row 1
    00, 07, 10, 13, 00, 00, 00, 00,  // Row 2
    00, 07, 10, 13, 00, 00, 00, 00,  // Row 3
    00, 07, 10, 13, 00, 00, 00, 00,  // Row 4
    00, 00, 00, 00, 00, 15, 15, 00,  // Row 5
    00, 00, 00, 00, 00, 15, 15, 00,  // Row 6
    00, 00, 00, 00, 00, 15, 15, 00,  // Row 7
    00, 00, 00, 00, 00, 00, 00, 00,  // Row 8
};

// Rough representation of 16 grayscale colours for drawing ASCII art
const char ascii_art[16] = {' ', '.', ':', '-', '=', 'z', 'X', 'Q',
                            'Z', '+', '*', '#', 'M', '%', '@', '$'};

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_IMAGE_PROCESSING_COMMONS_H_
