#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_TEMPLATED_STRUCT2_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_TEMPLATED_STRUCT2_H_

template <typename T>
struct Tag {
  T tag;
};

template <typename U, unsigned S>
struct Array {
  U data[S];
};

#define LEN 8

void convert(const Array<Tag<int>, LEN>& in,
             Tag<Array<short, LEN << 1>>& result);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_TEMPLATED_STRUCT2_H_
