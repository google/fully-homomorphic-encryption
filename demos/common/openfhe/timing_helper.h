#ifndef THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_DEMOS_COMMON_OPENFHE_TIMING_HELPER_H_
#define THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_DEMOS_COMMON_OPENFHE_TIMING_HELPER_H_

#include <map>
#include <string>
#include <vector>

#include "src/core/include/lattice/hal/lat-backend.h"
#include "src/pke/include/ciphertext-fwd.h"
#include "src/pke/include/cryptocontext-fwd.h"
#include "src/pke/include/key/privatekey-fwd.h"

using CiphertextT = lbcrypto::Ciphertext<lbcrypto::DCRTPoly>;
using CryptoContextT = lbcrypto::CryptoContext<lbcrypto::DCRTPoly>;
using PrivateKeyT = lbcrypto::PrivateKey<lbcrypto::DCRTPoly>;

void __heir_debug(CryptoContextT cc, PrivateKeyT sk, CiphertextT ct,
                  const std::map<std::string, std::string>& debugAttrMap);

void __heir_debug(CryptoContextT cc, PrivateKeyT sk,
                  std::vector<CiphertextT> cts,
                  const std::map<std::string, std::string>& debugAttrMap);

#endif  // THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_DEMOS_COMMON_OPENFHE_TIMING_HELPER_H_
