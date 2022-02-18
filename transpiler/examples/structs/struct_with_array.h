#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_STRUCT_WITH_ARRAY_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_STRUCT_WITH_ARRAY_H_

// Example demonstrating successful manipulation of arrays inside
// structs.
#define A_COUNT 3
#define B_COUNT 3
#define C_COUNT 3

struct Inner {
  int c[C_COUNT];
  short q;
};

struct StructWithArray {
  signed char c;
  Inner i;
  int a[A_COUNT];
  short b[B_COUNT];
  short z;
};

void NegateStructWithArray(StructWithArray &outer, int &other, Inner &inner);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_STRUCT_WITH_ARRAY_H_
