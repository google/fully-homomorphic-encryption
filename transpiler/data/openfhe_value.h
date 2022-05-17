#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_OPENFHE_VALUE_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_OPENFHE_VALUE_H_

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
#include "transpiler/data/cleartext_value.h"

struct OpenFhePrivateKeySet {
  lbcrypto::BinFHEContext cc;
  lbcrypto::LWEPrivateKey sk;
};

inline void OpenFheCopy(absl::Span<const lbcrypto::LWECiphertext> value,
                        const void*, absl::Span<lbcrypto::LWECiphertext> out) {
  for (int j = 0; j < value.size(); ++j) {
    out[j] = value[j];
  }
}

inline void OpenFheUnencrypted(absl::Span<const bool> value,
                               const lbcrypto::BinFHEContext* cc,
                               absl::Span<lbcrypto::LWECiphertext> out) {
  for (int j = 0; j < value.size(); ++j) {
    out[j] = cc->EvalConstant(value[j]);
  }
}

inline void OpenFheEncrypt(absl::Span<const bool> value,
                           lbcrypto::BinFHEContext cc,
                           lbcrypto::LWEPrivateKey sk,
                           absl::Span<lbcrypto::LWECiphertext> out) {
  for (int j = 0; j < value.size(); ++j) {
    out[j] = cc.Encrypt(sk, value[j], lbcrypto::FRESH);
  }
}

inline void OpenFheEncrypt(absl::Span<const bool> value,
                           const OpenFhePrivateKeySet* key,
                           absl::Span<lbcrypto::LWECiphertext> out) {
  return ::OpenFheEncrypt(value, key->cc, key->sk, out);
}

inline void OpenFheDecrypt(absl::Span<const lbcrypto::LWECiphertext> ciphertext,
                           lbcrypto::BinFHEContext cc,
                           lbcrypto::LWEPrivateKey sk,
                           absl::Span<bool> plaintext) {
  for (int j = 0; j < plaintext.size(); j++) {
    lbcrypto::LWEPlaintext bit;
    cc.Decrypt(sk, ciphertext[j], &bit);
    plaintext[j] = bit != 0;
  }
}

inline void OpenFheDecrypt(absl::Span<const lbcrypto::LWECiphertext> ciphertext,
                           const OpenFhePrivateKeySet* key,
                           absl::Span<bool> plaintext) {
  return OpenFheDecrypt(ciphertext, key->cc, key->sk, plaintext);
}

template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class OpenFheValueRef;

// FHE representation of a single object encoded as a bit array.
template <typename ValueType,
          std::enable_if_t<std::is_integral_v<ValueType>>* = nullptr>
class OpenFheValue {
 public:
  OpenFheValue(lbcrypto::BinFHEContext cc) : cc_(cc) {
    for (auto& bit : ciphertext_) {
      bit = cc.EvalConstant(false);
    }
  }

  OpenFheValue& operator=(const OpenFheValueRef<ValueType>& value) {
    ::OpenFheCopy(value.get(), nullptr, get());
    return *this;
  }

  static OpenFheValue<ValueType> Unencrypted(ValueType value,
                                             lbcrypto::BinFHEContext cc) {
    OpenFheValue<ValueType> plaintext(cc);
    plaintext.SetUnencrypted(value);
    return plaintext;
  }

  static OpenFheValue<ValueType> Encrypt(ValueType value,
                                         lbcrypto::BinFHEContext cc,
                                         lbcrypto::LWEPrivateKey sk) {
    OpenFheValue<ValueType> ciphertext(cc);
    ciphertext.SetEncrypted(value, sk);
    return ciphertext;
  }

  void SetUnencrypted(const ValueType& value) {
    ::OpenFheUnencrypted(EncodedValue<ValueType>(value).get(), cc_, get());
  }

  void SetEncrypted(const ValueType& value, lbcrypto::LWEPrivateKey sk) {
    ::OpenFheEncrypt(EncodedValue<ValueType>(value).get(), cc_, sk, get());
  }

  ValueType Decrypt(lbcrypto::LWEPrivateKey sk) {
    EncodedValue<ValueType> plaintext;
    ::OpenFheDecrypt(get(), cc_, sk, plaintext.get());
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
class OpenFheValueRef {
 public:
  OpenFheValueRef(absl::Span<lbcrypto::LWECiphertext> ciphertext,
                  lbcrypto::BinFHEContext cc)
      : ciphertext_(ciphertext), cc_(cc) {}
  OpenFheValueRef(OpenFheValue<ValueType>& value)
      : OpenFheValueRef(value.get(), value.context()) {}

  OpenFheValueRef& operator=(const OpenFheValueRef<ValueType>& value) {
    ::OpenFheCopy(value.get(), nullptr, get());
    return *this;
  }

  absl::Span<lbcrypto::LWECiphertext> get() { return ciphertext_; }
  absl::Span<const lbcrypto::LWECiphertext> get() const { return ciphertext_; }

  int32_t size() { return ciphertext_.size(); }

  lbcrypto::BinFHEContext context() { return cc_; }

 private:
  absl::Span<lbcrypto::LWECiphertext> ciphertext_;
  lbcrypto::BinFHEContext cc_;
};

template <typename ValueType, unsigned... Dimensions>
class OpenFheArrayRef;

template <typename ValueType, unsigned... Dimensions>
class OpenFheArray;

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_OPENFHE_VALUE_H_
