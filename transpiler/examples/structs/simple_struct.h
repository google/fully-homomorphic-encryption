#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_SIMPLE_STRUCT_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_SIMPLE_STRUCT_H_

// Very basic example demonstrating successful copy in of a struct type.

struct SimpleStruct {
  unsigned char a;
  int b;
  unsigned int c;
};

int SumSimpleStruct(SimpleStruct value);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_SIMPLE_STRUCT_H_
