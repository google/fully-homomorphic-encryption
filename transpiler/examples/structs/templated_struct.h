#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_TEMPLATED_STRUCT_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_TEMPLATED_STRUCT_H_

template <typename T, unsigned N>
struct StructWithArray {
  T data[N];
};

StructWithArray<int, 6> CollateThem(const StructWithArray<short, 3>&,
                                    const StructWithArray<char, 2>&);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_TEMPLATED_STRUCT_H_
