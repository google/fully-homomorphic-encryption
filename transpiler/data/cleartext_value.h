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
#include "include/ac_int.h"
#include "xls/common/logging/logging.h"
#include "xls/common/status/status_macros.h"

inline void CleartextCopy(absl::Span<const bool> src, const void*,
                          absl::Span<bool> dst) {
  for (int j = 0; j < dst.size(); ++j) {
    dst[j] = src[j];
  }
}

inline void CleartextEncode(absl::Span<const bool> value, const void*,
                            absl::Span<bool> out) {
  ::CleartextCopy(value, nullptr, out);
}

inline void CleartextDecode(absl::Span<const bool> ciphertext, const void*,
                            absl::Span<bool> plaintext) {
  // Encode and decode are the same function--a copy.
  ::CleartextCopy(ciphertext, nullptr, plaintext);
}

template <typename T>
inline void CleartextEncode(const T& value, absl::Span<bool> out) {
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
inline T CleartextDecode(absl::Span<bool> value) {
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

template <int Width, bool Sign>
inline void CleartextEncodeInteger(const ac_int<Width, Sign>& value,
                                   absl::Span<bool> out) {
  XLS_CHECK_EQ(Width, out.size());
  for (int j = 0; j < Width; ++j) {
    out[j] = value.template slc<1>(j);
  }
}

template <int Width, bool Sign>
inline ac_int<Width, Sign> CleartextDecodeInteger(absl::Span<bool> value) {
  XLS_CHECK_EQ(Width, value.size());
  ac_int<Width, Sign> val = 0;
  for (int j = 0; j < Width; j++) {
    val.set_slc(j, ac_int<1, false>(value[j]));
  }
  return val;
}

template <int Width, bool Signed = false>
class EncodedInteger {
 public:
  EncodedInteger() : array_(Width) {}
  EncodedInteger(ac_int<Width, Signed> value) : EncodedInteger() {
    this->Encode(value);
  }

  void Encode(const ac_int<Width, Signed>& value) {
    ::CleartextEncodeInteger<Width, Signed>(value, get());
  }

  ac_int<Width, Signed> Decode() {
    return ::CleartextDecodeInteger<Width, Signed>(get());
  }

  absl::Span<bool> get() { return absl::MakeSpan(array_); }
  absl::Span<const bool> get() const { return absl::MakeConstSpan(array_); }

  int32_t size() { return array_.size(); }

 private:
  absl::FixedArray<bool> array_;
};

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

  void Encode(const ValueType& value) { ::CleartextEncode(value, get()); }

  ValueType Decode() { return ::CleartextDecode<ValueType>(get()); }

  absl::Span<bool> get() { return absl::MakeSpan(array_); }

  int32_t size() { return array_.size(); }

 private:
  absl::FixedArray<bool> array_;
};

template <typename ValueType>
class Encoded;

template <typename ValueType>
class EncodedRef;

template <typename ValueType, unsigned... Dimensions>
class EncodedArray;

template <typename ValueType, unsigned... Dimensions>
class EncodedArrayRef;

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_CLEARTEXT_VALUE_H_
