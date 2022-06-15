#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_TFHE_VALUE_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_TFHE_VALUE_H_

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <memory>
#include <string>
#include <type_traits>

#include "absl/container/fixed_array.h"
#include "absl/types/span.h"
#include "include/ac_int.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
#include "transpiler/data/cleartext_value.h"
#include "xls/common/logging/logging.h"
#include "xls/common/status/status_macros.h"

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

inline void TfheCopy(absl::Span<const LweSample> value,
                     const TFheGateBootstrappingParameterSet* params,
                     absl::Span<LweSample> out) {
  for (int j = 0; j < out.size(); ++j) {
    lweCopy(&out[j], &value[j], params->in_out_params);
  }
}

inline void TfheUnencrypted(absl::Span<const bool> value,
                            const TFheGateBootstrappingCloudKeySet* key,
                            absl::Span<LweSample> out) {
  for (int j = 0; j < value.size(); ++j) {
    bootsCONSTANT(&out[j], value[j], key);
  }
}

inline void TfheEncrypt(absl::Span<const bool> value,
                        const TFheGateBootstrappingSecretKeySet* key,
                        absl::Span<LweSample> out) {
  for (int j = 0; j < out.size(); ++j) {
    bootsSymEncrypt(&out[j], value[j], key);
  }
}

inline void TfheDecrypt(absl::Span<const LweSample> ciphertext,
                        const TFheGateBootstrappingSecretKeySet* key,
                        absl::Span<bool> plaintext) {
  for (int j = 0; j < plaintext.size(); j++) {
    plaintext[j] = bootsSymDecrypt(&ciphertext[j], key) > 0;
  }
}

template <int Width, bool Signed>
inline void TfheEncryptInteger(const ac_int<Width, Signed>& value,
                               const TFheGateBootstrappingSecretKeySet* key,
                               absl::Span<LweSample> out) {
  XLS_CHECK_EQ(Width, out.size());
  for (int j = 0; j < Width; ++j) {
    bootsSymEncrypt(&out[j], value.template slc<1>(j), key);
  }
}

template <int Width, bool Signed>
inline ac_int<Width, Signed> TfheDecryptInteger(
    absl::Span<const LweSample> ciphertext,
    const TFheGateBootstrappingSecretKeySet* key) {
  XLS_CHECK_EQ(Width, ciphertext.size());
  ac_int<Width, Signed> val = 0;
  for (int j = 0; j < Width; j++) {
    val.set_slc(j, ac_int<1, false>(bootsSymDecrypt(&ciphertext[j], key) > 0));
  }
  return val;
}

template <int Width, bool Signed = false>
class TfheInteger {
 public:
  TfheInteger(const TFheGateBootstrappingParameterSet* params)
      : array_(new_gate_bootstrapping_ciphertext_array(Width, params),
               LweSampleArrayDeleter(Width)),
        params_(params) {}

  static TfheInteger<Width, Signed> Encrypt(
      const ac_int<Width, Signed>& value,
      const TFheGateBootstrappingSecretKeySet* key) {
    TfheInteger<Width, Signed> ciphertext(key->params);
    ciphertext.SetEncrypted(value, key);
    return ciphertext;
  }

  void SetEncrypted(const ac_int<Width, Signed>& value,
                    const TFheGateBootstrappingSecretKeySet* key) {
    ::TfheEncryptInteger(value, key, this->get());
  }

  const ac_int<Width, Signed> Decrypt(
      const TFheGateBootstrappingSecretKeySet* key) {
    return ::TfheDecryptInteger<Width, Signed>(this->get(), key);
  }

  absl::Span<LweSample> get() { return absl::MakeSpan(array_.get(), size()); }
  absl::Span<const LweSample> get() const {
    return absl::MakeConstSpan(array_.get(), size());
  }

  int32_t size() { return Width; }

 private:
  std::unique_ptr<LweSample, LweSampleArrayDeleter> array_;
  const TFheGateBootstrappingParameterSet* params_;
};

// TFHE representation of a single object encoded as a bit array.
template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class TfheValue {
 public:
  TfheValue(const TFheGateBootstrappingParameterSet* params)
      : array_(new_gate_bootstrapping_ciphertext_array(kBitWidth, params),
               LweSampleArrayDeleter(kBitWidth)),
        params_(params) {}

  static TfheValue<ValueType> Encrypt(
      ValueType value, const TFheGateBootstrappingSecretKeySet* key) {
    TfheValue<ValueType> ciphertext(key->params);
    ciphertext.SetEncrypted(value, key);
    return ciphertext;
  }

  void SetEncrypted(const ValueType& value,
                    const TFheGateBootstrappingSecretKeySet* key) {
    ::TfheEncrypt(EncodedValue<ValueType>(value).get(), key, this->get());
  }

  ValueType Decrypt(const TFheGateBootstrappingSecretKeySet* key) {
    EncodedValue<ValueType> plaintext;
    ::TfheDecrypt(this->get(), key, plaintext.get());
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

template <typename ValueType>
class Tfhe;

template <typename ValueType>
class TfheRef;

template <typename ValueType, unsigned... Dimensions>
class TfheArray;

template <typename ValueType, unsigned... Dimensions>
class TfheArrayRef;

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_TFHE_VALUE_H_
