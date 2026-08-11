#include "demos/cc_fraud/openfhe/debug_helper.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "demos/cc_fraud/openfhe/debug_reference.h"
#include "src/pke/include/ciphertext.h"
#include "src/pke/include/cryptocontext.h"
#include "src/pke/include/encoding/plaintext.h"

using PlaintextT = lbcrypto::Plaintext;

void __heir_debug(CryptoContextT cc, PrivateKeyT sk, CiphertextT ct,
                  const std::map<std::string, std::string>& debugAttrMap) {
  // Get op name
  std::string op_name = "unknown";
  if (debugAttrMap.find("debug.name") != debugAttrMap.end()) {
    op_name = debugAttrMap.at("debug.name");
  } else if (debugAttrMap.find("asm.op_name") != debugAttrMap.end()) {
    op_name = debugAttrMap.at("asm.op_name");
  }

  // Get current row index from environment
  const char* row_idx_str = std::getenv("HEIR_DEBUG_ROW_IDX");
  std::string row_key = "row_0";
  if (row_idx_str) {
    row_key = "row_" + std::string(row_idx_str);
  }

  std::cout << "\n[DEBUG] Step: " << op_name << " (" << row_key << ")"
            << std::endl;

  // Decrypt
  PlaintextT ptxt;
  cc->Decrypt(sk, ct, &ptxt);

  // We need to know how many elements to print.
  // We can get it from the reference data if available, or default to a small
  // number.
  size_t print_size = 5;
  bool has_ref = false;
  std::vector<float> ref_vals;

  if (debug_reference.find(row_key) != debug_reference.end()) {
    const auto& row_ref = debug_reference.at(row_key);
    if (row_ref.find(op_name) != row_ref.end()) {
      ref_vals = row_ref.at(op_name);
      print_size = ref_vals.size();
      has_ref = true;
    }
  }

  // Set length of plaintext to decode
  // If we don't have ref, we might decode too many slots.
  // In CKKS, we usually only care about the active slots.
  // If we don't have ref, we can try to get it from message.size if present.
  if (!has_ref && debugAttrMap.find("message.size") != debugAttrMap.end()) {
    print_size = std::stoul(debugAttrMap.at("message.size"));
  }

  ptxt->SetLength(print_size);

  // CKKS decrypts to CKKSPackedEncoding which can be cast to vector of complex
  // or double. In OpenFHE, CKKSPackedEncoding has GetRealHSV() to get real
  // values.
  auto fhe_vals = ptxt->GetRealPackedValue();

  // Print FHE values
  std::cout << "  FHE Decrypted (first min(5, size)): [";
  for (size_t i = 0; i < std::min(fhe_vals.size(), (size_t)5); ++i) {
    std::cout << fhe_vals[i]
              << (i == std::min(fhe_vals.size(), (size_t)5) - 1 ? "" : ", ");
  }
  if (fhe_vals.size() > 5) std::cout << ", ...";
  std::cout << "] (size: " << fhe_vals.size() << ")" << std::endl;

  // Print Scale
  double scale = ct->GetScalingFactor();
  std::cout << "  Scale: 2^" << std::log2(scale) << std::endl;

  // Compare with reference
  if (has_ref) {
    std::cout << "  Expected Ref  (first min(5, size)): [";
    for (size_t i = 0; i < std::min(ref_vals.size(), (size_t)5); ++i) {
      std::cout << ref_vals[i]
                << (i == std::min(ref_vals.size(), (size_t)5) - 1 ? "" : ", ");
    }
    if (ref_vals.size() > 5) std::cout << ", ...";
    std::cout << "]" << std::endl;

    // Calculate precision loss
    double max_abs_err = 0.0;
    for (size_t i = 0; i < std::min(fhe_vals.size(), ref_vals.size()); ++i) {
      double err = std::abs(fhe_vals[i] - ref_vals[i]);
      if (err > max_abs_err) {
        max_abs_err = err;
      }
    }
    std::cout << "  Max Abs Error: " << max_abs_err << std::endl;
    if (max_abs_err > 0.0) {
      std::cout << "  Precision Lost: 2^" << std::log2(max_abs_err) << " bits"
                << std::endl;
    } else {
      std::cout << "  Precision Lost: 0 bits (exact)" << std::endl;
    }

    // Sorted comparison to check if it is just a permutation
    if (fhe_vals.size() == ref_vals.size()) {
      std::vector<double> sorted_fhe = fhe_vals;
      std::vector<float> sorted_ref = ref_vals;
      std::sort(sorted_fhe.begin(), sorted_fhe.end());
      std::sort(sorted_ref.begin(), sorted_ref.end());

      double max_sorted_err = 0.0;
      for (size_t i = 0; i < sorted_fhe.size(); ++i) {
        double err = std::abs(sorted_fhe[i] - sorted_ref[i]);
        if (err > max_sorted_err) {
          max_sorted_err = err;
        }
      }
      std::cout << "  [Sorted Check] Max Abs Error: " << max_sorted_err
                << std::endl;
      if (max_sorted_err > 0.0) {
        std::cout << "  [Sorted Check] Precision Lost: 2^"
                  << std::log2(max_sorted_err) << " bits" << std::endl;
      } else {
        std::cout << "  [Sorted Check] Precision Lost: 0 bits (exact)"
                  << std::endl;
      }
    } else {
      std::cout << "  [Sorted Check] Skip (size mismatch: FHE "
                << fhe_vals.size() << " vs Ref " << ref_vals.size() << ")"
                << std::endl;
    }
  } else {
    std::cout << "  [WARNING] No reference data found for this step."
              << std::endl;
  }
}

void __heir_debug(CryptoContextT cc, PrivateKeyT sk,
                  std::vector<CiphertextT> cts,
                  const std::map<std::string, std::string>& debugAttrMap) {
  if (!cts.empty()) {
    __heir_debug(cc, sk, cts[0], debugAttrMap);
  }
}
