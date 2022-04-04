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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_PALISADE_DATA_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_PALISADE_DATA_H_

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
#include "transpiler/data/boolean_data.h"
#include "transpiler/data/cleartext_value.h"
#include "transpiler/data/palisade_value.h"

// PALISADE representation of an array of objects of a single type, encoded as
// a bit array.
template <typename ValueType>
class PalisadeArray<ValueType,
                    typename std::enable_if_t<std::is_integral_v<ValueType>>> {
 public:
  PalisadeArray(size_t length, lbcrypto::BinFHEContext cc)
      : length_(length), ciphertext_(bit_width()), cc_(cc) {
    for (auto& bit : ciphertext_) {
      bit = cc.EvalConstant(false);
    }
  }

  operator const PalisadeArrayRef<ValueType>() const& {
    return PalisadeArrayRef<ValueType>(absl::MakeConstSpan(ciphertext_), cc_);
  }
  operator PalisadeArrayRef<ValueType>() & {
    return PalisadeArrayRef<ValueType>(absl::MakeSpan(ciphertext_), cc_);
  }

  static PalisadeArray<ValueType> Unencrypted(
      absl::Span<const ValueType> plaintext, lbcrypto::BinFHEContext cc) {
    PalisadeArray<ValueType> shared_value(plaintext.length(), cc);
    shared_value.SetUnencrypted(plaintext);
    return plaintext;
  }

  static PalisadeArray<ValueType> Encrypt(absl::Span<const ValueType> plaintext,
                                          lbcrypto::BinFHEContext cc,
                                          lbcrypto::LWEPrivateKey sk) {
    PalisadeArray<ValueType> private_value(plaintext.length(), cc);
    private_value.SetEncrypted(plaintext, sk);
    return private_value;
  }

  void SetUnencrypted(absl::Span<const ValueType> plaintext) {
    assert(plaintext.length() == length_);
    ::Unencrypted(EncodedArray<ValueType>(plaintext).get(), cc_, get());
  }

  void SetEncrypted(absl::Span<const ValueType> plaintext,
                    lbcrypto::LWEPrivateKey sk) {
    assert(plaintext.length() == length_);
    ::Encrypt(EncodedArray<ValueType>(plaintext).get(), cc_, sk, get());
  }

  absl::FixedArray<ValueType> Decrypt(lbcrypto::LWEPrivateKey sk) const {
    EncodedArray<ValueType> encoded(length_);
    ::Decrypt(get(), cc_, sk, encoded.get());
    return encoded.Decode();
  }

  absl::Span<lbcrypto::LWECiphertext> get() {
    return absl::MakeSpan(ciphertext_);
  }
  absl::Span<const lbcrypto::LWECiphertext> get() const {
    return absl::MakeConstSpan(ciphertext_);
  }

  PalisadeValueRef<ValueType> operator[](size_t pos) {
    return {get().subspan(pos * kValueWidth, kValueWidth), context()};
  }

  size_t length() const { return length_; }
  size_t size() const { return length_; }

  size_t bit_width() const { return kValueWidth * length_; }

  lbcrypto::BinFHEContext context() { return cc_; }

 private:
  static constexpr size_t kValueWidth =
      std::is_same_v<ValueType, bool> ? 1 : 8 * sizeof(ValueType);

  size_t length_;
  absl::FixedArray<lbcrypto::LWECiphertext, kValueWidth> ciphertext_;
  lbcrypto::BinFHEContext cc_;
};

template <typename ValueType>
class PalisadeArrayRef<
    ValueType, typename std::enable_if_t<std::is_integral_v<ValueType>>> {
 public:
  PalisadeArrayRef(absl::Span<lbcrypto::LWECiphertext> data,
                   lbcrypto::BinFHEContext cc)
      : data_(data), cc_(cc) {}

  absl::Span<lbcrypto::LWECiphertext> get() { return data_; }
  absl::Span<const lbcrypto::LWECiphertext> get() const {
    return absl::MakeConstSpan(data_);
  }

 private:
  absl::Span<lbcrypto::LWECiphertext> data_;
  lbcrypto::BinFHEContext cc_;
};

// Represents a string as an PalisadeArray of the template parameter CharT.
// Corresponds to std::basic_string
template <typename CharT>
class PalisadeBasicString : public PalisadeArray<CharT> {
 public:
  using SizeType = typename std::basic_string<CharT>::size_type;

  using PalisadeArray<CharT>::PalisadeArray;

  static PalisadeBasicString<CharT> Unencrypted(
      absl::Span<const CharT> plaintext, lbcrypto::BinFHEContext cc) {
    PalisadeBasicString<CharT> shared_value(plaintext.length(), cc);
    shared_value.SetUnencrypted(plaintext);
    return shared_value;
  }

  static PalisadeBasicString<CharT> Encrypt(absl::Span<const CharT> plaintext,
                                            lbcrypto::BinFHEContext cc,
                                            lbcrypto::LWEPrivateKey sk) {
    PalisadeBasicString<CharT> private_value(plaintext.length(), cc);
    private_value.SetEncrypted(plaintext, sk);
    return private_value;
  }

  std::basic_string<CharT> Decrypt(lbcrypto::LWEPrivateKey sk) const {
    const absl::FixedArray<CharT> array = PalisadeArray<CharT>::Decrypt(sk);
    return std::basic_string<CharT>(array.begin(), array.end());
  }
};

// Instantiates a FHE representation of a string as an array of chars, which
// are themselves arrays of bits.
// Corresponds to std::string
using PalisadeString = PalisadeBasicString<char>;
using PalisadeStringRef = PalisadeArrayRef<char>;
// Corresponds to int
using PalisadeInt = PalisadeValue<int>;
// Corresponds to short
using PalisadeShort = PalisadeValue<short>;
// Corresponds to char
using PalisadeChar = PalisadeValue<char>;
using PalisadeCharRef = PalisadeValueRef<char>;
// Corresponds to bool
using PalisadeBit = PalisadeValue<bool>;
using PalisadeBool = PalisadeValue<bool>;

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_PALISADE_DATA_H_
