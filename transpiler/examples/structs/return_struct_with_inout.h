#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_RETURN_STRUCT_WITH_INOUT_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_RETURN_STRUCT_WITH_INOUT_H_

// Example demonstrating the ability to specify structs as return values and
// as in/out params.
struct Helper {
  int a;
  unsigned int b;
  long long c;
};

struct ReturnStruct {
  int a;
  Helper b;
  int c;
};

// Builds a ReturnStruct:
//  - "a" has the sum of the elements of input a.
//  - "b" is populated by the elements of input c.
//  - "c" has the sum of the elements of input b.
// And negates the contents of A and B (where appropriate).
ReturnStruct ConstructReturnStructWithInout(Helper& a, Helper& b,
                                            const Helper& c);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_RETURN_STRUCT_WITH_INOUT_H_
