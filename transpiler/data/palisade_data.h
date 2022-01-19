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

#ifndef FHE_DATA_H_
#define FHE_DATA_H_

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

struct PalisadePrivateKeySet {
  lbcrypto::BinFHEContext cc;
  lbcrypto::LWEPrivateKey sk;
};

inline void Copy(absl::Span<const lbcrypto::LWECiphertext> value,
                 absl::Span<lbcrypto::LWECiphertext> out) {
  for (int j = 0; j < value.size(); ++j) {
    out[j] = value[j];
  }
}

inline void Unencrypted(absl::Span<const bool> value,
                        lbcrypto::BinFHEContext cc,
                        absl::Span<lbcrypto::LWECiphertext> out) {
  for (int j = 0; j < value.size(); ++j) {
    out[j] = cc.EvalConstant(value[j]);
  }
}

inline void Unencrypted(absl::Span<const bool> value,
                        lbcrypto::BinFHEContext cc,
                        lbcrypto::LWECiphertext* out) {
  return Unencrypted(value, cc, absl::MakeSpan(out, value.size()));
}

inline void Encrypt(absl::Span<const bool> value, lbcrypto::BinFHEContext cc,
                    lbcrypto::LWEPrivateKey sk,
                    absl::Span<lbcrypto::LWECiphertext> out) {
  for (int j = 0; j < value.size(); ++j) {
    out[j] = cc.Encrypt(sk, value[j], lbcrypto::FRESH);
  }
}

inline void Encrypt(absl::Span<const bool> value, lbcrypto::BinFHEContext cc,
                    lbcrypto::LWEPrivateKey sk, lbcrypto::LWECiphertext* out) {
  return Encrypt(value, cc, sk, absl::MakeSpan(out, value.size()));
}

inline void Encrypt(absl::Span<const bool> value,
                    const PalisadePrivateKeySet* key,
                    lbcrypto::LWECiphertext* out) {
  return Encrypt(value, key->cc, key->sk, absl::MakeSpan(out, value.size()));
}

inline void Decrypt(absl::Span<const lbcrypto::LWECiphertext> ciphertext,
                    lbcrypto::BinFHEContext cc, lbcrypto::LWEPrivateKey sk,
                    absl::Span<bool> plaintext) {
  for (int j = 0; j < plaintext.size(); j++) {
    lbcrypto::LWEPlaintext bit;
    cc.Decrypt(sk, ciphertext[j], &bit);
    plaintext[j] = bit != 0;
  }
}

inline void Decrypt(lbcrypto::LWECiphertext* ciphertext,
                    lbcrypto::BinFHEContext cc, lbcrypto::LWEPrivateKey sk,
                    absl::Span<bool> plaintext) {
  return Decrypt(absl::MakeSpan(ciphertext, plaintext.size()), cc, sk,
                 plaintext);
}

inline void Decrypt(lbcrypto::LWECiphertext* ciphertext,
                    const PalisadePrivateKeySet* key,
                    absl::Span<bool> plaintext) {
  return Decrypt(absl::MakeSpan(ciphertext, plaintext.size()), key->cc, key->sk,
                 plaintext);
}

template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class PalisadeValueRef;

// FHE representation of a single object encoded as a bit array.
template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class PalisadeValue {
 public:
  PalisadeValue(lbcrypto::BinFHEContext cc) : cc_(cc) {
    for (auto& bit : ciphertext_) {
      bit = cc.EvalConstant(false);
    }
  }

  static PalisadeValue<ValueType> Unencrypted(ValueType value,
                                              lbcrypto::BinFHEContext cc) {
    PalisadeValue<ValueType> plaintext(cc);
    plaintext.SetUnencrypted(value);
    return plaintext;
  }

  static PalisadeValue<ValueType> Encrypt(ValueType value,
                                          lbcrypto::BinFHEContext cc,
                                          lbcrypto::LWEPrivateKey sk) {
    PalisadeValue<ValueType> ciphertext(cc);
    ciphertext.SetEncrypted(value, sk);
    return ciphertext;
  }

  PalisadeValue& operator=(const PalisadeValueRef<ValueType>& value) {
    ::Copy(value.get(), get());
    return *this;
  }

  void SetUnencrypted(const ValueType& value) {
    ::Unencrypted(EncodedValue<ValueType>(value), cc_, get());
  }

  void SetEncrypted(const ValueType& value, lbcrypto::LWEPrivateKey sk) {
    ::Encrypt(EncodedValue<ValueType>(value), cc_, sk, get());
  }

  ValueType Decrypt(lbcrypto::LWEPrivateKey sk) {
    EncodedValue<ValueType> plaintext;
    ::Decrypt(get(), cc_, sk, plaintext);
    return plaintext.Decode();
  }

  absl::Span<lbcrypto::LWECiphertext> get() {
    return absl::MakeSpan(ciphertext_);
  }
  absl::Span<const lbcrypto::LWECiphertext> get() const {
    return absl::MakeConstSpan(ciphertext_);
  }

  int32_t size() { return ciphertext_.size(); }

  lbcrypto::BinFHEContext context() { return cc_; }

 private:
  static constexpr size_t kValueWidth =
      std::is_same_v<ValueType, bool> ? 1 : 8 * sizeof(ValueType);

  std::array<lbcrypto::LWECiphertext, kValueWidth> ciphertext_;
  lbcrypto::BinFHEContext cc_;
};

// Reference to an FHE representation of a single object encoded as a bit array.
template <typename ValueType, std::enable_if_t<std::is_integral_v<ValueType>>*>
class PalisadeValueRef {
 public:
  PalisadeValueRef(absl::Span<lbcrypto::LWECiphertext> ciphertext,
                   lbcrypto::BinFHEContext cc)
      : ciphertext_(ciphertext), cc_(cc) {}
  PalisadeValueRef(const PalisadeValue<ValueType>& value)
      : PalisadeValueRef(value.get(), value.context()) {}

  PalisadeValueRef& operator=(const PalisadeValueRef<ValueType>& value) {
    ::Copy(value.get(), get());
    return *this;
  }

  void SetUnencrypted(const ValueType& value) {
    ::Unencrypted(EncodedValue<ValueType>(value), cc_, ciphertext_);
  }

  void SetEncrypted(const ValueType& value, lbcrypto::LWEPrivateKey sk) {
    ::Encrypt(EncodedValue<ValueType>(value), cc_, sk, ciphertext_);
  }

  ValueType Decrypt(lbcrypto::LWEPrivateKey sk) {
    EncodedValue<ValueType> plaintext;
    ::Decrypt(ciphertext_, cc_, sk, plaintext);
    return plaintext.Decode();
  }

  absl::Span<lbcrypto::LWECiphertext> get() { return ciphertext_; }
  absl::Span<const lbcrypto::LWECiphertext> get() const { return ciphertext_; }

  int32_t size() { return ciphertext_.size(); }

  lbcrypto::BinFHEContext context() { return cc_; }

 private:
  absl::Span<lbcrypto::LWECiphertext> ciphertext_;
  lbcrypto::BinFHEContext cc_;
};

// FHE representation of an array of objects of a single type, encoded as
// a bit array.
template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class PalisadeArray {
 public:
  PalisadeArray(size_t length, lbcrypto::BinFHEContext cc)
      : length_(length), ciphertext_(bit_width()), cc_(cc) {
    for (auto& bit : ciphertext_) {
      bit = cc.EvalConstant(false);
    }
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
    ::Unencrypted(EncodedArray<ValueType>(plaintext), cc_, get());
  }

  void SetEncrypted(absl::Span<const ValueType> plaintext,
                    lbcrypto::LWEPrivateKey sk) {
    assert(plaintext.length() == length_);
    ::Encrypt(EncodedArray<ValueType>(plaintext), cc_, sk, get());
  }

  absl::FixedArray<ValueType> Decrypt(lbcrypto::LWEPrivateKey sk) const {
    EncodedArray<ValueType> encoded(length_);
    ::Decrypt(get(), cc_, sk, encoded);
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
// Corresponds to int
using PalisadeInt = PalisadeValue<int>;
// Corresponds to short
using PalisadeShort = PalisadeValue<short>;
// Corresponds to char
using PalisadeChar = PalisadeValue<char>;
// Corresponds to bool
using PalisadeBit = PalisadeValue<bool>;
using PalisadeBool = PalisadeValue<bool>;

#endif  // FHE_DATA_H_
