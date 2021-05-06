#include "return_struct.h"

#pragma hls_top
ReturnStruct ConstructReturnStruct(unsigned char a, Embedded b,
                                   unsigned char c) {
  ReturnStruct ret;
  ret.a = a;
  ret.b = b;
  ret.c = c;
  return ret;
}
