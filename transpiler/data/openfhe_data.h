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
#include "transpiler/data/boolean_data.h"
#include "transpiler/data/cleartext_value.h"
#include "transpiler/data/openfhe_value.h"

// OpenFHE representation of an array of objects of a single type, encoded as
// a bit array.
template <typename ValueType>
class OpenFheArray<ValueType,
                   typename std::enable_if_t<std::is_integral_v<ValueType>>> {
 public:
  OpenFheArray(size_t length, lbcrypto::BinFHEContext cc)
      : length_(length), ciphertext_(bit_width()), cc_(cc) {
    for (auto& bit : ciphertext_) {
      bit = cc.EvalConstant(false);
    }
  }

  operator const OpenFheArrayRef<ValueType>() const& {
    return OpenFheArrayRef<ValueType>(absl::MakeConstSpan(ciphertext_), cc_);
  }
  operator OpenFheArrayRef<ValueType>() & {
    return OpenFheArrayRef<ValueType>(absl::MakeSpan(ciphertext_), cc_);
  }

  static OpenFheArray<ValueType> Unencrypted(
      absl::Span<const ValueType> plaintext, lbcrypto::BinFHEContext cc) {
    OpenFheArray<ValueType> shared_value(plaintext.length(), cc);
    shared_value.SetUnencrypted(plaintext);
    return plaintext;
  }

  static OpenFheArray<ValueType> Encrypt(absl::Span<const ValueType> plaintext,
                                         lbcrypto::BinFHEContext cc,
                                         lbcrypto::LWEPrivateKey sk) {
    OpenFheArray<ValueType> private_value(plaintext.length(), cc);
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

  OpenFheValueRef<ValueType> operator[](size_t pos) {
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
class OpenFheArrayRef<
    ValueType, typename std::enable_if_t<std::is_integral_v<ValueType>>> {
 public:
  OpenFheArrayRef(absl::Span<lbcrypto::LWECiphertext> data,
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

// Represents a string as an OpenFheArray of the template parameter CharT.
// Corresponds to std::basic_string
template <typename CharT>
class OpenFheBasicString : public OpenFheArray<CharT> {
 public:
  using SizeType = typename std::basic_string<CharT>::size_type;

  using OpenFheArray<CharT>::OpenFheArray;

  static OpenFheBasicString<CharT> Unencrypted(
      absl::Span<const CharT> plaintext, lbcrypto::BinFHEContext cc) {
    OpenFheBasicString<CharT> shared_value(plaintext.length(), cc);
    shared_value.SetUnencrypted(plaintext);
    return shared_value;
  }

  static OpenFheBasicString<CharT> Encrypt(absl::Span<const CharT> plaintext,
                                           lbcrypto::BinFHEContext cc,
                                           lbcrypto::LWEPrivateKey sk) {
    OpenFheBasicString<CharT> private_value(plaintext.length(), cc);
    private_value.SetEncrypted(plaintext, sk);
    return private_value;
  }

  std::basic_string<CharT> Decrypt(lbcrypto::LWEPrivateKey sk) const {
    const absl::FixedArray<CharT> array = OpenFheArray<CharT>::Decrypt(sk);
    return std::basic_string<CharT>(array.begin(), array.end());
  }
};

// Instantiates a FHE representation of a string as an array of chars, which
// are themselves arrays of bits.
// Corresponds to std::string
using OpenFheString = OpenFheBasicString<char>;
using OpenFheStringRef = OpenFheArrayRef<char>;
// Corresponds to int
using OpenFheInt = OpenFheValue<int>;
// Corresponds to short
using OpenFheShort = OpenFheValue<short>;
// Corresponds to char
using OpenFheChar = OpenFheValue<char>;
using OpenFheCharRef = OpenFheValueRef<char>;
// Corresponds to bool
using OpenFheBit = OpenFheValue<bool>;
using OpenFheBool = OpenFheValue<bool>;

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_OPENFHE_DATA_H_
