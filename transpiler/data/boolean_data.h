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

template <typename T>
inline void Encode(const T& value, absl::Span<bool> out) {
  if constexpr (std::is_same_v<T, bool>) {
    out[0] = value;
  } else {
    const auto unsigned_value = absl::bit_cast<std::make_unsigned_t<T>>(value);
    for (int j = 0; j < 8 * sizeof(T); ++j) {
      out[j] = ((unsigned_value >> j) & 1) != 0;
    }
  }
}

template <typename T>
inline T Decode(absl::Span<bool> value) {
  if constexpr (std::is_same_v<T, bool>) {
    return value[0];
  } else {
    using UnsignedT = std::make_unsigned_t<T>;

    UnsignedT unsigned_value = 0;
    for (int j = 0; j < 8 * sizeof(T); j++) {
      unsigned_value |= static_cast<UnsignedT>(value[j]) << j;
    }
    return absl::bit_cast<T>(unsigned_value);
  }
}

template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class EncodedValueRef;

template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class EncodedValue {
 public:
  EncodedValue()
      : array_(std::is_same_v<ValueType, bool> ? 1 : 8 * sizeof(ValueType)) {}
  EncodedValue(ValueType value) : EncodedValue() { Encode(value); }
  EncodedValue(absl::Span<bool> encoded) : EncodedValue() {
    std::copy(encoded.begin(), encoded.end(), array_.begin());
  }

  EncodedValue& operator=(const EncodedValueRef<ValueType>& value) {
    std::copy(value.get().cbegin(), value.get().cend(), get().begin());
    return *this;
  }

  operator const EncodedValueRef<ValueType>() const& {
    return EncodedValueRef<ValueType>(array_);
  }
  operator EncodedValueRef<ValueType>() & {
    return EncodedValueRef<ValueType>(absl::MakeSpan(array_));
  }

  void Encode(const ValueType& value) { ::Encode(value, get()); }

  ValueType Decode() { return ::Decode<ValueType>(get()); }

  absl::Span<bool> get() { return absl::MakeSpan(array_); }

  int32_t size() { return array_.size(); }

 private:
  absl::FixedArray<bool> array_;
};

template <typename ValueType, std::enable_if_t<std::is_integral_v<ValueType>>*>
class EncodedValueRef {
 public:
  EncodedValueRef(absl::Span<bool> data) : data_(data) {}

  EncodedValueRef& operator=(const EncodedValueRef<ValueType>& value) {
    std::copy(value.get().cbegin(), value.get().cend(), get());
    return *this;
  }

  absl::Span<bool> get() { return data_; }
  absl::Span<const bool> get() const { return data_; }

 private:
  absl::Span<bool> data_;
};

template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class EncodedArrayRef;

// FHE representation of an array of objects of a single type, encoded as
// a bit array.
template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class EncodedArray {
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

template <typename ValueType, std::enable_if_t<std::is_integral_v<ValueType>>*>
class EncodedArrayRef {
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
