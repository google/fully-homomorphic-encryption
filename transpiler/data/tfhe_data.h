// Copyright 2021 Google LLC
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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_TFHE_DATA_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_TFHE_DATA_H_

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <array>
#include <memory>
#include <string>
#include <type_traits>

#include "absl/container/fixed_array.h"
#include "absl/types/span.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
#include "transpiler/data/cleartext_data.h"
#include "transpiler/data/cleartext_value.h"
#include "transpiler/data/primitives_tfhe.types.h"
#include "transpiler/data/tfhe_value.h"

// Corresponds to std::string
using TfheString = TfheArray<char>;
using TfheStringRef = TfheArrayRef<char>;
// Corresponds to char
using TfheChar = Tfhe<char>;
using TfheCharRef = TfheRef<char>;
// Corresponds to signed/unsigned char
using TfheSignedChar = Tfhe<signed char>;
using TfheSignedCharRef = TfheRef<signed char>;
using TfheUnsignedChar = Tfhe<unsigned char>;
using TfheUnsignedCharRef = TfheRef<unsigned char>;
// Corresponds to long
using TfheLong = Tfhe<signed long>;
using TfheLongRef = TfheRef<signed long>;
using TfheUnsignedLong = Tfhe<unsigned long>;
using TfheUnsignedLongRef = TfheRef<unsigned long>;
// Corresponds to int
using TfheInt = Tfhe<signed int>;
using TfheIntRef = TfheRef<signed int>;
using TfheUnsignedInt = Tfhe<unsigned int>;
using TfheUnsignedIntRef = TfheRef<unsigned int>;
// Corresponds to short
using TfheShort = Tfhe<signed short>;
using TfheShortRef = TfheRef<signed short>;
using TfheUnsignedShort = Tfhe<unsigned short>;
using TfheUnsignedShortRef = TfheRef<unsigned short>;
// Corresponds to bool
using TfheBit = Tfhe<bool>;
using TfheBitRef = TfheRef<bool>;
using TfheBool = Tfhe<bool>;
using TfheBoolRef = TfheRef<bool>;

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_TFHE_DATA_H_
