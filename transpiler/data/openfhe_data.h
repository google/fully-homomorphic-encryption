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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_OPENFHE_DATA_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_OPENFHE_DATA_H_

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <array>
#include <memory>
#include <string>
#include <type_traits>

#include "absl/container/fixed_array.h"
#include "absl/types/span.h"
#include "palisade/binfhe/binfhecontext.h"
#include "transpiler/data/cleartext_data.h"
#include "transpiler/data/cleartext_value.h"
#include "transpiler/data/openfhe_value.h"
#include "transpiler/data/primitives_openfhe.types.h"

// Corresponds to std::string
using OpenFheString = OpenFheArray<char>;
using OpenFheStringRef = OpenFheArrayRef<char>;
// Corresponds to char
using OpenFheChar = OpenFhe<char>;
using OpenFheCharRef = OpenFheRef<char>;
// Corresponds to signed/unsigned char
using OpenFheSignedChar = OpenFhe<signed char>;
using OpenFheSignedCharRef = OpenFheRef<signed char>;
using OpenFheUnsignedChar = OpenFhe<unsigned char>;
using OpenFheUnsignedCharRef = OpenFheRef<unsigned char>;
// Corresponds to long
using OpenFheLong = OpenFhe<signed long>;
using OpenFheLongRef = OpenFheRef<signed long>;
using OpenFheUnsignedLong = OpenFhe<unsigned long>;
using OpenFheUnsignedLongRef = OpenFheRef<unsigned long>;
// Corresponds to int
using OpenFheInt = OpenFhe<signed int>;
using OpenFheIntRef = OpenFheRef<signed int>;
using OpenFheUnsignedInt = OpenFhe<unsigned int>;
using OpenFheUnsignedIntRef = OpenFheRef<unsigned int>;
// Corresponds to short
using OpenFheShort = OpenFhe<signed short>;
using OpenFheShortRef = OpenFheRef<signed short>;
using OpenFheUnsignedShort = OpenFhe<unsigned short>;
using OpenFheUnsignedShortRef = OpenFheRef<unsigned short>;
// Corresponds to bool
using OpenFheBit = OpenFhe<bool>;
using OpenFheBitRef = OpenFheRef<bool>;
using OpenFheBool = OpenFhe<bool>;
using OpenFheBoolRef = OpenFheRef<bool>;

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_OPENFHE_DATA_H_
