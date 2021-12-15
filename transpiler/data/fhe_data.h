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
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
#include "transpiler/data/boolean_data.h"

struct TFheGateBootstrappingParameterSetDeleter {
  void operator()(TFheGateBootstrappingParameterSet* params_) const {
    delete_gate_bootstrapping_parameters(params_);
  }
};

// Parameters used to generate a key set.
class TFHEParameters {
 public:
  TFHEParameters(int32_t minimum_lambda)
      : params_(new_default_gate_bootstrapping_parameters(minimum_lambda)) {}

  TFheGateBootstrappingParameterSet* get() { return params_.get(); }

  operator TFheGateBootstrappingParameterSet*() { return params_.get(); }

 private:
  std::unique_ptr<TFheGateBootstrappingParameterSet,
                  TFheGateBootstrappingParameterSetDeleter>
      params_;
};

struct TFheGateBootstrappingSecretKeySetDeleter {
  void operator()(TFheGateBootstrappingSecretKeySet* bk_) const {
    delete_gate_bootstrapping_secret_keyset(bk_);
  }
};

// Class used to generate a secret key.
class TFHESecretKeySet {
 public:
  template <size_t N>
  TFHESecretKeySet(const TFheGateBootstrappingParameterSet* params,
                   std::array<uint32_t, N> seed = {}) {
    if (!seed.empty()) {
      tfhe_random_generator_setSeed(seed.data(), seed.size());
    }
    bk_.reset(new_random_gate_bootstrapping_secret_keyset(params));
  }

  TFheGateBootstrappingSecretKeySet* get() { return bk_.get(); }
  operator TFheGateBootstrappingSecretKeySet*() { return bk_.get(); }

  const TFheGateBootstrappingCloudKeySet* cloud() { return &bk_->cloud; }
  const TFheGateBootstrappingParameterSet* params() { return bk_->params; }

 private:
  std::unique_ptr<TFheGateBootstrappingSecretKeySet,
                  TFheGateBootstrappingSecretKeySetDeleter>
      bk_;
};

// TODO: This should be part of the LweSample class.
struct LweSampleSingletonDeleter {
  void operator()(LweSample* lwe_sample) const {
    delete_gate_bootstrapping_ciphertext(lwe_sample);
  }
};

struct LweSampleArrayDeleter {
  int32_t width_;

  LweSampleArrayDeleter(int32_t width) : width_(width) {}

  void operator()(LweSample* lwe_sample) const {
    delete_gate_bootstrapping_ciphertext_array(width_, lwe_sample);
  }
};

inline void Copy(const LweSample* value, int32_t width,
                 const TFheGateBootstrappingParameterSet* params,
                 LweSample* out) {
  for (int j = 0; j < width; ++j) {
    lweCopy(&out[j], &value[j], params->in_out_params);
  }
}

inline void Unencrypted(absl::Span<const bool> value,
                        const TFheGateBootstrappingCloudKeySet* key,
                        LweSample* out) {
  for (int j = 0; j < value.size(); ++j) {
    bootsCONSTANT(&out[j], value[j], key);
  }
}

inline void Encrypt(absl::Span<const bool> value,
                    const TFheGateBootstrappingSecretKeySet* key,
                    LweSample* out) {
  for (int j = 0; j < value.size(); ++j) {
    bootsSymEncrypt(&out[j], value[j], key);
  }
}

inline void Decrypt(LweSample* ciphertext,
                    const TFheGateBootstrappingSecretKeySet* key,
                    absl::Span<bool> plaintext) {
  for (int j = 0; j < plaintext.size(); j++) {
    plaintext[j] = bootsSymDecrypt(&ciphertext[j], key) > 0;
  }
}

template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class FheValueRef;

// FHE representation of a single object encoded as a bit array.
template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class FheValue {
 public:
  FheValue(const TFheGateBootstrappingParameterSet* params)
      : array_(new_gate_bootstrapping_ciphertext_array(kBitWidth, params),
               LweSampleArrayDeleter(kBitWidth)),
        params_(params) {}

  static FheValue<ValueType> Unencrypted(
      ValueType value, const TFheGateBootstrappingCloudKeySet* key) {
    FheValue<ValueType> plaintext(key->params);
    plaintext.SetUnencrypted(value, key);
    return plaintext;
  }

  static FheValue<ValueType> Encrypt(
      ValueType value, const TFheGateBootstrappingSecretKeySet* key) {
    FheValue<ValueType> ciphertext(key->params);
    ciphertext.SetEncrypted(value, key);
    return ciphertext;
  }

  FheValue& operator=(const FheValueRef<ValueType>& value) {
    ::Copy(value.get(), value.size(), params(), get());
    return *this;
  }

  operator const FheValueRef<ValueType>() const& {
    return FheValueRef<ValueType>(array_.get(), params_);
  }
  operator FheValueRef<ValueType>() & {
    return FheValueRef<ValueType>(array_.get(), params_);
  }

  void SetUnencrypted(const ValueType& value,
                      const TFheGateBootstrappingCloudKeySet* key) {
    ::Unencrypted(EncodedValue<ValueType>(value), key, array_.get());
  }

  void SetEncrypted(const ValueType& value,
                    const TFheGateBootstrappingSecretKeySet* key) {
    ::Encrypt(EncodedValue<ValueType>(value), key, array_.get());
  }

  ValueType Decrypt(const TFheGateBootstrappingSecretKeySet* key) {
    EncodedValue<ValueType> plaintext;
    ::Decrypt(array_.get(), key, plaintext);
    return plaintext.Decode();
  }

  absl::Span<LweSample> get() { return absl::MakeSpan(array_.get(), size()); }
  absl::Span<const LweSample> get() const {
    return absl::MakeConstSpan(array_.get(), size());
  }

  int32_t size() { return kBitWidth; }

  const TFheGateBootstrappingParameterSet* params() { return params_; }

 private:
  static constexpr int32_t kBitWidth =
      std::is_same_v<ValueType, bool> ? 1 : 8 * sizeof(ValueType);

  std::unique_ptr<LweSample, LweSampleArrayDeleter> array_;
  const TFheGateBootstrappingParameterSet* params_;
};

// Reference to an FHE representation of a single object encoded as a bit array.
template <typename ValueType, std::enable_if_t<std::is_integral_v<ValueType>>*>
class FheValueRef {
 public:
  FheValueRef(LweSample* array, const TFheGateBootstrappingParameterSet* params)
      : array_(array), params_(params) {}

  FheValueRef& operator=(const FheValueRef<ValueType>& value) {
    ::Copy(value.get().data(), size(), params(), this->get().data());
    return *this;
  }

  void SetUnencrypted(const ValueType& value,
                      const TFheGateBootstrappingCloudKeySet* key) {
    ::Unencrypted(EncodedValue<ValueType>(value), key, this->get());
  }

  void SetEncrypted(const ValueType& value,
                    const TFheGateBootstrappingSecretKeySet* key) {
    ::Encrypt(EncodedValue<ValueType>(value), key, this->get());
  }

  ValueType Decrypt(const TFheGateBootstrappingSecretKeySet* key) {
    EncodedValue<ValueType> plaintext;
    ::Decrypt(this->get(), key, plaintext);
    return plaintext.Decode();
  }

  int32_t size() const { return kBitWidth; }

  absl::Span<LweSample> get() { return absl::MakeSpan(array_, size()); }
  absl::Span<const LweSample> get() const {
    return absl::MakeConstSpan(array_, size());
  }

  const TFheGateBootstrappingParameterSet* params() { return params_; }

 private:
  static constexpr int32_t kBitWidth =
      std::is_same_v<ValueType, bool> ? 1 : 8 * sizeof(ValueType);

  LweSample* array_;
  const TFheGateBootstrappingParameterSet* params_;
};

template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class FheArrayRef;

// FHE representation of an array of objects of a single type, encoded as
// a bit array.
template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class FheArray {
 public:
  FheArray(int32_t length, const TFheGateBootstrappingParameterSet* params)
      : length_(length),
        array_(new_gate_bootstrapping_ciphertext_array(bit_width(), params),
               LweSampleArrayDeleter(kValueWidth * length)),
        params_(params) {}

  static FheArray<ValueType> Unencrypted(
      absl::Span<const ValueType> plaintext,
      const TFheGateBootstrappingCloudKeySet* key) {
    FheArray<ValueType> shared_value(plaintext.length(), key->params);
    shared_value.SetUnencrypted(plaintext, key);
    return plaintext;
  }

  static FheArray<ValueType> Encrypt(
      absl::Span<const ValueType> plaintext,
      const TFheGateBootstrappingSecretKeySet* key) {
    FheArray<ValueType> private_value(plaintext.length(), key->params);
    private_value.SetEncrypted(plaintext, key);
    return private_value;
  }

  operator const FheArrayRef<ValueType>() const& {
    return FheArrayRef<ValueType>(length_, array_.get(), params_);
  }
  operator FheArrayRef<ValueType>() & {
    return FheArrayRef<ValueType>(length_, array_.get(), params_);
  }

  void SetUnencrypted(absl::Span<const ValueType> plaintext,
                      const TFheGateBootstrappingCloudKeySet* key) {
    assert(plaintext.length() == length_);
    ::Unencrypted(EncodedArray<ValueType>(plaintext), key, array_.get());
  }

  void SetEncrypted(absl::Span<const ValueType> plaintext,
                    const TFheGateBootstrappingSecretKeySet* key) {
    assert(plaintext.length() == length_);
    ::Encrypt(EncodedArray<ValueType>(plaintext), key, array_.get());
  }

  absl::FixedArray<ValueType> Decrypt(
      const TFheGateBootstrappingSecretKeySet* key) const {
    EncodedArray<ValueType> encoded(length_);
    ::Decrypt(array_.get(), key, encoded);
    return encoded.Decode();
  }

  absl::Span<LweSample> get() { return absl::MakeSpan(array_.get(), size()); }
  absl::Span<const LweSample> get() const {
    return absl::MakeConstSpan(array_.get(), size());
  }

  FheValueRef<ValueType> operator[](int32_t pos) {
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

// Reference to an FHE representation of an array of objects of a single type,
// encoded as a bit array.
template <typename ValueType, std::enable_if_t<std::is_integral_v<ValueType>>*>
class FheArrayRef {
 public:
  FheArrayRef(int32_t length, LweSample* array,
              const TFheGateBootstrappingParameterSet* params)
      : length_(length), array_(array), params_(params) {}

  FheArrayRef& operator=(const FheArrayRef<ValueType>& value) {
    ::Copy(value.get(), size(), params(), this->get());
    return *this;
  }

  void SetUnencrypted(absl::Span<const ValueType> plaintext,
                      const TFheGateBootstrappingCloudKeySet* key) {
    assert(plaintext.length() == length_);
    ::Unencrypted(EncodedArray<ValueType>(plaintext), key, this->get());
  }

  void SetEncrypted(absl::Span<const ValueType> plaintext,
                    const TFheGateBootstrappingSecretKeySet* key) {
    assert(plaintext.length() == length_);
    ::Encrypt(EncodedArray<ValueType>(plaintext), key, this->get());
  }

  absl::FixedArray<ValueType> Decrypt(
      const TFheGateBootstrappingSecretKeySet* key) const {
    EncodedArray<ValueType> encoded(length_);
    ::Decrypt(this->get(), key, encoded);
    return encoded.Decode();
  }

  absl::Span<LweSample> get() { return absl::MakeSpan(array_, size()); }
  absl::Span<const LweSample> get() const {
    return absl::MakeConstSpan(array_, size());
  }

  FheValueRef<ValueType> operator[](int32_t pos) {
    return {&(this->get())[pos * kValueWidth], params()};
  }

  int32_t length() const { return length_; }
  int32_t size() const { return length_; }

  int32_t bit_width() const { return kValueWidth * length_; }

  const TFheGateBootstrappingParameterSet* params() { return params_; }

 private:
  static constexpr int32_t kValueWidth =
      std::is_same_v<ValueType, bool> ? 1 : 8 * sizeof(ValueType);

  const int32_t length_;
  LweSample* array_;
  const TFheGateBootstrappingParameterSet* params_;
};

class FheCharRef : public FheValueRef<char> {
 public:
  using FheValueRef<char>::FheValueRef;

  operator FheValueRef<signed char>() {
    return FheValueRef<signed char>(this->get().data(), this->params());
  }
  operator FheValueRef<unsigned char>() {
    return FheValueRef<unsigned char>(this->get().data(), this->params());
  }
};

// Represents a character as an FheValue of the template parameter CharT.
// Allow implicit reference as plain, signed, or unsigned CharT.
class FheChar : public FheValue<char> {
 public:
  using FheValue<char>::FheValue;

  static FheChar Encrypt(char value,
                         const TFheGateBootstrappingSecretKeySet* key) {
    FheChar ciphertext(key->params);
    ciphertext.SetEncrypted(value, key);
    return ciphertext;
  }

  static FheChar Unencrypted(char value,
                             const TFheGateBootstrappingCloudKeySet* key) {
    FheChar plaintext(key->params);
    plaintext.SetUnencrypted(value, key);
    return plaintext;
  }

  operator FheCharRef() & {
    return FheCharRef(this->get().data(), this->params());
  }
  operator FheValueRef<signed char>() & { return ((FheCharRef) * this); }
  operator FheValueRef<unsigned char>() & { return ((FheCharRef) * this); }
};

// Represents a string as an FheArray of the template parameter CharT.
// Corresponds to std::basic_string
template <typename CharT>
class FheBasicString : public FheArray<CharT> {
 public:
  using FheArray<CharT>::FheArray;

  static FheBasicString<CharT> Unencrypted(
      absl::Span<const CharT> plaintext,
      const TFheGateBootstrappingCloudKeySet* key) {
    FheBasicString<CharT> shared_value(plaintext.length(), key->params);
    shared_value.SetUnencrypted(plaintext, key);
    return shared_value;
  }

  static FheBasicString<CharT> Encrypt(
      absl::Span<const CharT> plaintext,
      const TFheGateBootstrappingSecretKeySet* key) {
    FheBasicString<CharT> private_value(plaintext.length(), key->params);
    private_value.SetEncrypted(plaintext, key);
    return private_value;
  }

  std::basic_string<CharT> Decrypt(
      const TFheGateBootstrappingSecretKeySet* key) const {
    const absl::FixedArray<CharT> array = FheArray<CharT>::Decrypt(key);
    return std::basic_string<CharT>(array.begin(), array.end());
  }

  template <std::enable_if_t<std::is_same_v<CharT, char>, void>* = nullptr>
  operator FheArrayRef<signed char>() {
    return FheArrayRef<signed char>(this->length(), this->get(),
                                    this->params());
  }

  template <std::enable_if_t<std::is_same_v<CharT, char>, void>* = nullptr>
  operator FheArrayRef<unsigned char>() {
    return FheArrayRef<unsigned char>(this->length(), this->get(),
                                      this->params());
  }

  template <std::enable_if_t<std::is_same_v<CharT, char>, void>* = nullptr>
  FheCharRef operator[](int32_t pos) {
    FheValueRef<CharT> ref = FheArray<CharT>::operator[](pos);
    return {ref.get().data(), ref.params()};
  }
};

// Instantiates a FHE representation of a string as an array of chars, which
// are themselves arrays of bits.
// Corresponds to std::string
using FheString = FheBasicString<char>;
// Corresponds to int
using FheInt = FheValue<int>;
// Corresponds to short
using FheShort = FheValue<short>;
// Corresponds to bool
using FheBit = FheValue<bool>;
using FheBool = FheValue<bool>;

#endif  // FHE_DATA_H_
