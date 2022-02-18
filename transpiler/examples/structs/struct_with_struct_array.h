#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_STRUCT_WITH_STRUCT_ARRAY_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_STRUCT_WITH_STRUCT_ARRAY_H_

// Example demonstrating successful manipulation of arrays inside
// structs.
#define ELEMENT_COUNT 4

struct Simple {
  char a[ELEMENT_COUNT];
};

struct StructWithStructArray {
  Simple a[ELEMENT_COUNT];
  int b[ELEMENT_COUNT];
};

// Negates the elements in the input array.
StructWithStructArray NegateStructWithStructArray(StructWithStructArray input);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_STRUCT_WITH_STRUCT_ARRAY_H_
