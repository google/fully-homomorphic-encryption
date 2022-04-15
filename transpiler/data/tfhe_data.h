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
#include "transpiler/data/boolean_data.h"
#include "transpiler/data/cleartext_value.h"
#include "transpiler/data/tfhe_value.h"

// TFHE representation of an array of objects of a single type, encoded as
// a bit array.
template <typename ValueType>
class TfheArray<ValueType,
                typename std::enable_if_t<std::is_integral_v<ValueType>>> {
 public:
  TfheArray(int32_t length, const TFheGateBootstrappingParameterSet* params)
      : length_(length),
        array_(new_gate_bootstrapping_ciphertext_array(bit_width(), params),
               LweSampleArrayDeleter(kValueWidth * length)),
        params_(params) {}

  static TfheArray<ValueType> Unencrypted(
      absl::Span<const ValueType> plaintext,
      const TFheGateBootstrappingCloudKeySet* key) {
    TfheArray<ValueType> shared_value(plaintext.length(), key->params);
    shared_value.SetUnencrypted(plaintext, key);
    return plaintext;
  }

  static TfheArray<ValueType> Encrypt(
      absl::Span<const ValueType> plaintext,
      const TFheGateBootstrappingSecretKeySet* key) {
    TfheArray<ValueType> private_value(plaintext.length(), key->params);
    private_value.SetEncrypted(plaintext, key);
    return private_value;
  }

  operator const TfheArrayRef<ValueType>() const& {
    return TfheArrayRef<ValueType>(length_, array_.get(), params_);
  }
  operator TfheArrayRef<ValueType>() & {
    return TfheArrayRef<ValueType>(length_, array_.get(), params_);
  }

  void SetUnencrypted(absl::Span<const ValueType> plaintext,
                      const TFheGateBootstrappingCloudKeySet* key) {
    assert(plaintext.length() == length_);
    ::TfheUnencrypted(EncodedArray<ValueType>(plaintext).get(), key, get());
  }

  void SetEncrypted(absl::Span<const ValueType> plaintext,
                    const TFheGateBootstrappingSecretKeySet* key) {
    assert(plaintext.length() == length_);
    ::TfheEncrypt(EncodedArray<ValueType>(plaintext).get(), key, get());
  }

  absl::FixedArray<ValueType> Decrypt(
      const TFheGateBootstrappingSecretKeySet* key) const {
    EncodedArray<ValueType> encoded(length_);
    ::TfheDecrypt(get(), key, encoded.get());
    return encoded.Decode();
  }

  absl::Span<LweSample> get() { return absl::MakeSpan(array_.get(), size()); }
  absl::Span<const LweSample> get() const {
    return absl::MakeConstSpan(array_.get(), size());
  }

  TfheValueRef<ValueType> operator[](int32_t pos) {
    return {&(array_.get())[pos * kValueWidth], params()};
  }

  int32_t length() const { return length_; }
  int32_t size() const { return bit_width(); }

  int32_t bit_width() const { return kValueWidth * length_; }

  const TFheGateBootstrappingParameterSet* params() { return params_; }

 private:
  static constexpr int32_t kValueWidth =
      std::is_same_v<ValueType, bool> ? 1 : 8 * sizeof(ValueType);

  const int32_t length_;
  std::unique_ptr<LweSample, LweSampleArrayDeleter> array_;
  const TFheGateBootstrappingParameterSet* params_;
};

// Reference to a TFHE representation of an array of objects of a single type,
// encoded as a bit array.
template <typename ValueType>
class TfheArrayRef<ValueType,
                   typename std::enable_if_t<std::is_integral_v<ValueType>>> {
 public:
  TfheArrayRef(int32_t length, LweSample* array,
               const TFheGateBootstrappingParameterSet* params)
      : length_(length), array_(array), params_(params) {}

  absl::Span<LweSample> get() { return absl::MakeSpan(array_, size()); }
  absl::Span<const LweSample> get() const {
    return absl::MakeConstSpan(array_, size());
  }

  int32_t size() const { return bit_width(); }
  int32_t bit_width() const { return kValueWidth * length_; }

  const TFheGateBootstrappingParameterSet* params() { return params_; }

 private:
  static constexpr int32_t kValueWidth =
      std::is_same_v<ValueType, bool> ? 1 : 8 * sizeof(ValueType);

  const int32_t length_;
  LweSample* array_;
  const TFheGateBootstrappingParameterSet* params_;
};

class TfheCharRef : public TfheValueRef<char> {
 public:
  using TfheValueRef<char>::TfheValueRef;

  operator TfheValueRef<signed char>() {
    return TfheValueRef<signed char>(this->get().data(), this->params());
  }
  operator TfheValueRef<unsigned char>() {
    return TfheValueRef<unsigned char>(this->get().data(), this->params());
  }
};

// Represents a character as a TfheValue of the template parameter CharT.
// Allow implicit reference as plain, signed, or unsigned CharT.
class TfheChar : public TfheValue<char> {
 public:
  using TfheValue<char>::TfheValue;

  static TfheChar Encrypt(char value,
                          const TFheGateBootstrappingSecretKeySet* key) {
    TfheChar ciphertext(key->params);
    ciphertext.SetEncrypted(value, key);
    return ciphertext;
  }

  static TfheChar Unencrypted(char value,
                              const TFheGateBootstrappingCloudKeySet* key) {
    TfheChar plaintext(key->params);
    plaintext.SetUnencrypted(value, key);
    return plaintext;
  }

  operator TfheCharRef() & {
    return TfheCharRef(this->get().data(), this->params());
  }
  operator TfheValueRef<signed char>() & { return ((TfheCharRef) * this); }
  operator TfheValueRef<unsigned char>() & { return ((TfheCharRef) * this); }
};

// Represents a string as a TfheArray of the template parameter CharT.
// Corresponds to std::basic_string
template <typename CharT>
class TfheBasicString : public TfheArray<CharT> {
 public:
  using TfheArray<CharT>::TfheArray;

  static TfheBasicString<CharT> Unencrypted(
      absl::Span<const CharT> plaintext,
      const TFheGateBootstrappingCloudKeySet* key) {
    TfheBasicString<CharT> shared_value(plaintext.length(), key->params);
    shared_value.SetUnencrypted(plaintext, key);
    return shared_value;
  }

  static TfheBasicString<CharT> Encrypt(
      absl::Span<const CharT> plaintext,
      const TFheGateBootstrappingSecretKeySet* key) {
    TfheBasicString<CharT> private_value(plaintext.length(), key->params);
    private_value.SetEncrypted(plaintext, key);
    return private_value;
  }

  std::basic_string<CharT> Decrypt(
      const TFheGateBootstrappingSecretKeySet* key) const {
    const absl::FixedArray<CharT> array = TfheArray<CharT>::Decrypt(key);
    return std::basic_string<CharT>(array.begin(), array.end());
  }

  template <std::enable_if_t<std::is_same_v<CharT, char>, void>* = nullptr>
  operator TfheArrayRef<signed char>() {
    return TfheArrayRef<signed char>(this->length(), this->get(),
                                     this->params());
  }

  template <std::enable_if_t<std::is_same_v<CharT, char>, void>* = nullptr>
  operator TfheArrayRef<unsigned char>() {
    return TfheArrayRef<unsigned char>(this->length(), this->get(),
                                       this->params());
  }

  template <std::enable_if_t<std::is_same_v<CharT, char>, void>* = nullptr>
  TfheCharRef operator[](int32_t pos) {
    TfheValueRef<CharT> ref = TfheArray<CharT>::operator[](pos);
    return {ref.get().data(), ref.params()};
  }
};

// Instantiates a TFHE representation of a string as an array of chars, which
// are themselves arrays of bits.
// Corresponds to std::string
using TfheString = TfheBasicString<char>;
using TfheStringRef = TfheArrayRef<char>;
// Corresponds to int
using TfheInt = TfheValue<int>;
// Corresponds to short
using TfheShort = TfheValue<short>;
// Corresponds to bool
using TfheBit = TfheValue<bool>;
using TfheBool = TfheValue<bool>;

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_TFHE_DATA_H_
