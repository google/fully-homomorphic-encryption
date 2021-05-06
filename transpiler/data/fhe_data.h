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

// FHE representation of a single bit.
class FheBit {
 public:
  FheBit(const TFheGateBootstrappingParameterSet* params)
      : value_(new_gate_bootstrapping_ciphertext(params)) {}

  FheBit(bool value, const TFheGateBootstrappingCloudKeySet* key)
      : FheBit(key->params) {
    Set(value, key);
  }
  FheBit(bool value, const TFheGateBootstrappingSecretKeySet* key)
      : FheBit(key->params) {
    Encrypt(value, key);
  }

  void Set(bool value, const TFheGateBootstrappingCloudKeySet* key) {
    bootsCONSTANT(value_.get(), value, key);
  }

  void Encrypt(bool value, const TFheGateBootstrappingSecretKeySet* key) {
    bootsSymEncrypt(value_.get(), value, key);
  }

  bool Decrypt(const TFheGateBootstrappingSecretKeySet* key) {
    return bootsSymDecrypt(value_.get(), key) > 0;
  }

  LweSample* get() { return value_.get(); }
  const LweSample* get() const { return value_.get(); }

 private:
  // Pointer to a single value.
  std::unique_ptr<LweSample, LweSampleSingletonDeleter> value_;
};

struct LweSampleArrayDeleter {
  int width_;

  LweSampleArrayDeleter(int width) : width_(width) {}

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
          std::enable_if_t<std::is_integral_v<ValueType> &&
                           !std::is_same_v<ValueType, bool>>* = nullptr>
class FheValueRef;

// FHE representation of a single object encoded as a bit array.
template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType> &&
                           !std::is_same_v<ValueType, bool>>* = nullptr>
class FheValue {
 public:
  FheValue(const TFheGateBootstrappingParameterSet* params)
      : width_(8 * sizeof(ValueType)),
        array_(new_gate_bootstrapping_ciphertext_array(width_, params),
               LweSampleArrayDeleter(width_)),
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

  LweSample* get() { return array_.get(); }
  const LweSample* get() const { return array_.get(); }

  int32_t size() { return width_; }

  const TFheGateBootstrappingParameterSet* params() { return params_; }

 private:
  int32_t width_;
  std::unique_ptr<LweSample, LweSampleArrayDeleter> array_;
  const TFheGateBootstrappingParameterSet* params_;
};

// Reference to an FHE representation of a single object encoded as a bit array.
template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType> &&
                           !std::is_same_v<ValueType, bool>>*>
class FheValueRef {
 public:
  FheValueRef(LweSample* array, const TFheGateBootstrappingParameterSet* params)
      : params_(params), width_(8 * sizeof(ValueType)), array_(array) {}
  FheValueRef(const FheValue<ValueType>& value)
      : FheValueRef(value.get(), value.params()) {}

  FheValueRef& operator=(const FheValueRef<ValueType>& value) {
    ::Copy(value.get(), size(), params(), this->get());
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

  LweSample* get() { return array_; }
  const LweSample* get() const { return array_; }

  int32_t size() { return width_; }

  const TFheGateBootstrappingParameterSet* params() { return params_; }

 private:
  int32_t width_;
  LweSample* array_;
  const TFheGateBootstrappingParameterSet* params_;
};

// FHE representation of an array of objects of a single type, encoded as
// a bit array.
template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType> &&
                           !std::is_same_v<ValueType, bool>>* = nullptr>
class FheArray {
 public:
  FheArray(size_t length, const TFheGateBootstrappingParameterSet* params)
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

  LweSample* get() { return array_.get(); }
  const LweSample* get() const { return array_.get(); }

  FheValueRef<ValueType> operator[](int32_t pos) {
    return {&(array_.get())[pos * kValueWidth], params()};
  }

  size_t length() const { return length_; }
  size_t size() const { return length_; }

  size_t bit_width() const { return kValueWidth * length_; }

  const TFheGateBootstrappingParameterSet* params() { return params_; }

 private:
  static constexpr size_t kValueWidth = 8 * sizeof(ValueType);

  size_t length_;
  std::unique_ptr<LweSample, LweSampleArrayDeleter> array_;
  const TFheGateBootstrappingParameterSet* params_;
};

// Represents a string as an FheArray of the template parameter CharT.
// Corresponds to std::basic_string
template <typename CharT>
class FheBasicString : public FheArray<CharT> {
 public:
  using SizeType = typename std::basic_string<CharT>::size_type;

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
};

// Instantiates a FHE representation of a string as an array of chars, which
// are themselves arrays of bits.
// Corresponds to std::string
using FheString = FheBasicString<char>;
// Corresponds to int
using FheInt = FheValue<int>;
// Corresponds to short
using FheShort = FheValue<short>;
// Corresponds to char
using FheChar = FheValue<char>;

#endif  // FHE_DATA_H_
