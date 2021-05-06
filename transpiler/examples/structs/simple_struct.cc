#include "simple_struct.h"

#pragma hls_top
int SumSimpleStruct(SimpleStruct value) { return value.a + value.b + value.c; }
