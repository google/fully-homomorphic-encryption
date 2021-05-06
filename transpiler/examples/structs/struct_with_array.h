#ifndef TRANSPILER_EXAMPLES_STRUCTS_STRUCT_WITH_ARRAY_H_
#define TRANSPILER_EXAMPLES_STRUCTS_STRUCT_WITH_ARRAY_H_

// Example demonstrating successful manipulation of arrays inside
// structs.
#define A_COUNT 3
#define B_COUNT 4

struct StructWithArray {
  int a[3];
  short b[4];
  int c;
};

StructWithArray NegateStructWithArray(StructWithArray input);

#endif  // TRANSPILER_EXAMPLES_STRUCTS_STRUCT_WITH_ARRAY_H_
