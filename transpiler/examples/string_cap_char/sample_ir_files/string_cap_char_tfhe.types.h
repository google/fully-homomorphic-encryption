#include <cstdint>
#include <memory>

#include "tfhe/tfhe.h"
#include "transpiler/data/fhe_data.h"
#include "transpiler/examples/string_cap_char/string_cap_char.h"

class FheState {
 public:
  FheState(const TFheGateBootstrappingParameterSet* params)
      : data_(new_gate_bootstrapping_ciphertext_array(bit_width_, params),
              LweSampleArrayDeleter(bit_width_)) {}

  // We set values here directly, instead of using FheValue, since FheValue
  // types own their arrays, whereas we'd need to own them here as
  // contiguously-allocated chunks. (We could modify them to use borrowed data,
  // but it'd be more work than this). For structure types, though, we do
  // enable setting on "borrowed" data to enable recursive setting.
  void SetUnencrypted(const State& value,
                      const TFheGateBootstrappingCloudKeySet* key) {
    SetUnencryptedInternal(value, key, data_.get());
  }

  void SetEncrypted(const State& value,
                    const TFheGateBootstrappingSecretKeySet* key) {
    SetEncryptedInternal(value, key, data_.get());
  }

  State Decrypt(const TFheGateBootstrappingSecretKeySet* key) {
    State result;
    DecryptInternal(key, data_.get(), &result);
    return result;
  }

  static void BorrowedSetUnencrypted(
      const State& value, const TFheGateBootstrappingCloudKeySet* key,
      LweSample* data) {
    SetUnencryptedInternal(value, key, data);
  }

  static void BorrowedSetEncrypted(const State& value,
                                   const TFheGateBootstrappingSecretKeySet* key,
                                   LweSample* data) {
    SetEncryptedInternal(value, key, data);
  }

  static void BorrowedDecrypt(const TFheGateBootstrappingSecretKeySet* key,
                              LweSample* data, State* result) {
    DecryptInternal(key, data, result);
  }

  LweSample* get() { return data_.get(); }
  size_t bit_width() { return bit_width_; }

 private:
  size_t bit_width_ = 1;

  static void SetUnencryptedInternal(
      const State& value, const TFheGateBootstrappingCloudKeySet* key,
      LweSample* data) {
    ::Unencrypted(EncodedValue<bool>(value.last_was_space_), key, data);
    data += 1;
  }

  static void SetEncryptedInternal(const State& value,
                                   const TFheGateBootstrappingSecretKeySet* key,
                                   LweSample* data) {
    ::Encrypt(EncodedValue<bool>(value.last_was_space_), key, data);
    data += 1;
  }

  static void DecryptInternal(const TFheGateBootstrappingSecretKeySet* key,
                              LweSample* data, State* result) {
    EncodedValue<bool> encoded_last_was_space_;
    ::Decrypt(data, key, encoded_last_was_space_);
    data += 1;
    result->last_was_space_ = encoded_last_was_space_.Decode();
  }

  std::unique_ptr<LweSample, LweSampleArrayDeleter> data_;
};
