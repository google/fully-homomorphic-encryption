#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_PALISADE_VALUE_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_PALISADE_VALUE_H_

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

  PalisadeValue& operator=(const PalisadeValueRef<ValueType>& value) {
    ::Copy(value.get(), get());
    return *this;
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

  void SetUnencrypted(const ValueType& value) {
    ::Unencrypted(EncodedValue<ValueType>(value).get(), cc_, get());
  }

  void SetEncrypted(const ValueType& value, lbcrypto::LWEPrivateKey sk) {
    ::Encrypt(EncodedValue<ValueType>(value).get(), cc_, sk, get());
  }

  ValueType Decrypt(lbcrypto::LWEPrivateKey sk) {
    EncodedValue<ValueType> plaintext;
    ::Decrypt(get(), cc_, sk, plaintext.get());
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
  PalisadeValueRef(PalisadeValue<ValueType>& value)
      : PalisadeValueRef(value.get(), value.context()) {}

  PalisadeValueRef& operator=(const PalisadeValueRef<ValueType>& value) {
    ::Copy(value.get(), get());
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

template <typename ValueType, typename Enable = void, unsigned... Dimensions>
class PalisadeArrayRef {};

template <typename ValueType, typename Enable = void, unsigned... Dimensions>
class PalisadeArray {};

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_PALISADE_VALUE_H_
