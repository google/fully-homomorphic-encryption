#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_CLEARTEXT_VALUE_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_CLEARTEXT_VALUE_H_

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

template <typename ValueType, typename Enable = void, unsigned... Dimensions>
class EncodedArrayRef {};

template <typename ValueType, typename Enable = void, unsigned... Dimensions>
class EncodedArray {};

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_CLEARTEXT_VALUE_H_
