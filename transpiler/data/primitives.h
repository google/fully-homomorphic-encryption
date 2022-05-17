#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_PRIMITIVES_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_PRIMITIVES_H_

struct PrimitiveBool {
  bool v_;
};

struct PrimitiveChar {
  char v_;
};

struct PrimitiveSignedChar {
  signed char v_;
};

struct PrimitiveUnsignedChar {
  unsigned char v_;
};

struct PrimitiveSignedShort {
  signed short v_;
};

using PrimitiveShort = PrimitiveSignedShort;

struct PrimitiveUnsignedShort {
  unsigned short v_;
};

struct PrimitiveSignedInt {
  signed int v_;
};

using PrimitiveInt = PrimitiveSignedInt;

struct PrimitiveUnsignedInt {
  unsigned int v_;
};

struct PrimitiveSignedLong {
  signed long v_;
};

using PrimitiveLong = PrimitiveSignedLong;

struct PrimitiveUnsignedLong {
  unsigned long v_;
};

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_PRIMITIVES_H_
