// cOPYRIGht 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_SQRT_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_SQRT_H_

#ifdef __SYNTHESIS__
#include "xls_int.h"
#else
#include "transpiler/data/fhe_xls_int.h"
#endif

XlsInt<22, false> isqrt(XlsInt<22, false> num);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_SQRT_H_
