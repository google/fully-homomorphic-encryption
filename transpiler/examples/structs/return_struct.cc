#include "return_struct.h"

#pragma hls_top
ReturnStruct ConstructReturnStruct(char a, Embedded b, char c) {
  ReturnStruct ret;
  ret.a = a;
  ret.b = b;
  ret.c = c;
  return ret;
}
