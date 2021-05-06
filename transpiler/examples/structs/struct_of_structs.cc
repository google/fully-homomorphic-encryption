#include "struct_of_structs.h"

#pragma hls_top
int SumStructOfStructs(StructOfStructs value) {
  int result = 0;
  result += value.x;
  result += value.a.a;
  result += value.a.b;
  result += value.a.c;
  result += value.b.a;
  result += value.b.b;
  result += value.b.c;
  result += value.c.b.a;
  result += value.c.b.b;
  result += value.c.b.c;
  result += value.d.x;
  result += value.i.h.g.f.e.d.x;
  return result;
}
