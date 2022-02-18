#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_STRUCT_OF_STRUCTS_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_STRUCT_OF_STRUCTS_H_

// Example demonstrating that structs can be composed of other struct types (and
// still transpile, etc. correctly).
struct BaseA {
  unsigned char a;
  int b;
  unsigned short c;
};

struct BaseB {
  int a;
  unsigned short b;
  unsigned char c;
};

struct BaseC {
  BaseB b;
};

// Test long & dumb chains.
struct BaseD {
  unsigned char x;
};

struct BaseE {
  struct BaseD d;
};

struct BaseF {
  unsigned char x;
  struct BaseE e;
};

struct BaseG {
  struct BaseF f;
};

struct BaseH {
  struct BaseG g;
};

struct BaseI {
  struct BaseH h;
};

struct StructOfStructs {
  unsigned char x;
  BaseA a;
  BaseB b;
  BaseC c;
  BaseD d;
  BaseI i;
};

// Calculates the sum of each [transitive] element in a StructOfStructs.
int SumStructOfStructs(StructOfStructs value);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_STRUCT_OF_STRUCTS_H_
