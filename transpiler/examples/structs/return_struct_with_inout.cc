#include "return_struct_with_inout.h"

#pragma hls_top
ReturnStruct ConstructReturnStructWithInout(Helper& a, Helper& b,
                                            const Helper& c) {
  Helper ret_b;
  ret_b.a = c.a;
  ret_b.b = c.b;
  ret_b.c = c.c;

  ReturnStruct ret;
  ret.a = a.a + a.b + a.c;
  ret.b = ret_b;
  ret.c = b.a + b.b + b.c;

  a.a = -a.a;
  a.c = -a.c;
  b.a = -b.a;
  b.c = -b.c;
  return ret;
}
