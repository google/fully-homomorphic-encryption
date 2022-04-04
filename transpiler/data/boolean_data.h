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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_BOOLEAN_DATA_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_BOOLEAN_DATA_H_

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <string>
#include <type_traits>

#include "absl/base/casts.h"
#include "absl/container/fixed_array.h"
#include "absl/types/span.h"
#include "transpiler/data/cleartext_value.h"

// FHE representation of an array of objects of a single type, encoded as
// a bit array.
template <typename ValueType>
class EncodedArray<ValueType,
                   typename std::enable_if_t<std::is_integral_v<ValueType>>> {
 public:
  EncodedArray(size_t length) : length_(length), array_(kValueWidth * length) {}

  EncodedArray(absl::Span<const ValueType> plaintext)
      : EncodedArray(plaintext.size()) {
    Encode(plaintext);
  }

  operator const EncodedArrayRef<ValueType>() const& {
    return EncodedArrayRef<ValueType>(absl::MakeConstSpan(array_));
  }
  operator EncodedArrayRef<ValueType>() & {
    return EncodedArrayRef<ValueType>(absl::MakeSpan(array_));
  }

  void Encode(absl::Span<const ValueType> plaintext) {
    assert(plaintext.size() == length_);
    for (int j = 0; j < plaintext.size(); j++) {
      ::Encode(plaintext[j], (*this)[j].get());
    }
  }

  absl::FixedArray<ValueType> Decode() {
    absl::FixedArray<ValueType> plaintext(length_);
    for (int j = 0; j < length_; j++) {
      plaintext[j] = ::Decode<ValueType>((*this)[j].get());
    }
    return plaintext;
  }

  absl::Span<bool> get() { return absl::MakeSpan(array_); }
  absl::Span<const bool> get() const { return absl::MakeSpan(array_); }

  EncodedValueRef<ValueType> operator[](size_t pos) {
    return EncodedValueRef<ValueType>(
        absl::MakeSpan(array_).subspan(pos * kValueWidth, kValueWidth));
  }

  size_t length() const { return length_; }
  size_t size() const { return length_; }

  size_t bit_width() const { return array_.size(); }

 private:
  using UnsignedType = std::make_unsigned_t<ValueType>;
  static constexpr size_t kValueWidth =
      std::is_same_v<ValueType, bool> ? 1 : 8 * sizeof(ValueType);

  size_t length_;
  absl::FixedArray<bool> array_;
};

template <typename ValueType>
class EncodedArrayRef<
    ValueType, typename std::enable_if_t<std::is_integral_v<ValueType>>> {
 public:
  EncodedArrayRef(absl::Span<bool> data) : data_(std::move(data)) {}
  absl::Span<bool> get() { return data_; }
  absl::Span<const bool> get() const { return absl::MakeConstSpan(data_); }

 private:
  absl::Span<bool> data_;
};

// Represents a string as an EncodedArray of the template parameter CharT.
// Corresponds to std::basic_string
template <typename CharT>
class EncodedBasicString : public EncodedArray<CharT> {
 public:
  using SizeType = typename std::basic_string<CharT>::size_type;

  using EncodedArray<CharT>::EncodedArray;

  std::basic_string<CharT> Decode() {
    const absl::FixedArray<CharT> v = EncodedArray<CharT>::Decode();
    return std::basic_string<CharT>(v.begin(), v.end());
  }
};

// Instantiates a FHE representation of a string as an array of chars, which
// are themselves arrays of bits.
// Corresponds to std::string
using EncodedString = EncodedBasicString<char>;
using EncodedStringRef = EncodedArrayRef<char>;

using EncodedBool = EncodedValue<bool>;

using EncodedInt = EncodedValue<int>;

using EncodedChar = EncodedValue<char>;
using EncodedCharRef = EncodedValueRef<char>;

using EncodedShort = EncodedValue<short>;

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_BOOLEAN_DATA_H_
