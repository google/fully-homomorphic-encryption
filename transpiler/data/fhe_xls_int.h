#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_FHE_XLS_INT_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_FHE_XLS_INT_H_

#include "include/ac_int.h"
#include "transpiler/data/cleartext_value.h"

// This struct definition mirrors the way XlsInt is defined in the XLScc
// (synthesis-only) header xls_int.h.  When XLScc processes a file that has an
// XlsInt instance in it, it will generate metadata for that instance.  Since
// XlsInt is currently defined as a subclass of XlsIntBase, the metdata will
// contain a reference to a field with the name "XlsIntBase".  When the struct
// transpiler parses this metadata, it will expect to see a class that follows
// the same layout.  For additional details see comments in the struct
// transpiler itself.
//
// Any change to the implementation of XlsInt in xls_int.h that affects the
// layout will break the transpiler build and require a change to this struct.

template <int Width, bool Signed>
struct XlsInt {
  XlsInt(ac_int<Width, Signed> value = 0) : XlsIntBase(value) {}
  EncodedInteger<Width, Signed> XlsIntBase;
  operator ac_int<Width, Signed>() { return XlsIntBase.Decode(); }
};

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_FHE_XLS_INT_H_
