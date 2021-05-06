#ifndef TRANSPILER_EXAMPLES_STRUCTS_SIMPLE_STRUCT_H_
#define TRANSPILER_EXAMPLES_STRUCTS_SIMPLE_STRUCT_H_

// Very basic example demonstrating successful copy in of a struct type.

struct SimpleStruct {
  unsigned char a;
  int b;
  unsigned int c;
};

int SumSimpleStruct(SimpleStruct value);

#endif  // TRANSPILER_EXAMPLES_STRUCTS_SIMPLE_STRUCT_H_
