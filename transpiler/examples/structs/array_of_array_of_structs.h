#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_ARRAY_OF_ARRAY_OF_STRUCTS_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_ARRAY_OF_ARRAY_OF_STRUCTS_H_

// Example demonstrating successful manipulation of arrays of arrays...of
// structs.
// The index sizes are low to keep compilation times from exploding.
#define A_ELEMENTS 2
#define B_ELEMENTS 1
#define C_ELEMENTS 2
#define D_ELEMENTS 4

struct Simple {
  char a[D_ELEMENTS];
  char b;
};

struct Base {
  Simple a[A_ELEMENTS][B_ELEMENTS][C_ELEMENTS];
};

// Doubles the elements in the input array.
Base DoubleBase(Base input);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_ARRAY_OF_ARRAY_OF_STRUCTS_H_
