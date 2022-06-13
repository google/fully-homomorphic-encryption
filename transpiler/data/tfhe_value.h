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
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
#include "transpiler/data/cleartext_value.h"

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

template <typename ValueType, unsigned... Dimensions>
class TfheArrayRef;

template <typename ValueType, unsigned... Dimensions>
class TfheArray;

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_TFHE_VALUE_H_
