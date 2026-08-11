#include "demos/common/openfhe/timing_helper.h"

#include <chrono>  // NOLINT(build/c++11)
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "src/pke/include/cryptocontext.h"

thread_local static std::chrono::high_resolution_clock::time_point g_last_time;
thread_local static std::chrono::high_resolution_clock::time_point g_start_time;
thread_local static bool g_started = false;

void __heir_debug(CryptoContextT cc, PrivateKeyT sk, CiphertextT ct,
                  const std::map<std::string, std::string>& debugAttrMap) {
  std::string op_name = "unknown";
  if (debugAttrMap.find("debug.name") != debugAttrMap.end()) {
    op_name = debugAttrMap.at("debug.name");
  } else if (debugAttrMap.find("asm.op_name") != debugAttrMap.end()) {
    op_name = debugAttrMap.at("asm.op_name");
  }

  auto now = std::chrono::high_resolution_clock::now();

  if (!g_started || op_name == "input") {
    g_start_time = now;
    g_last_time = now;
    g_started = true;
    std::cout << "[TIMING] Evaluation started at operator: " << op_name
              << std::endl;
    std::cout << "[TIMING] Ring dimension: " << cc->GetRingDimension()
              << std::endl;
    const auto& elementParams = cc->GetCryptoParameters()->GetElementParams();
    std::cout << "[TIMING] Moduli count: " << elementParams->GetParams().size()
              << std::endl;
    for (size_t i = 0; i < elementParams->GetParams().size(); ++i) {
      std::cout
          << "[TIMING]   Modulus " << i << ": "
          << elementParams->GetParams()[i]->GetModulus() << " (~"
          << std::round(std::log2(
                 elementParams->GetParams()[i]->GetModulus().ConvertToDouble()))
          << " bits)" << std::endl;
    }
  } else {
    double section_duration =
        std::chrono::duration<double>(now - g_last_time).count();
    double total_duration =
        std::chrono::duration<double>(now - g_start_time).count();
    std::cout << "[TIMING] After operator: " << std::left << std::setw(16)
              << op_name << " | Section duration: " << std::fixed
              << std::setprecision(4) << std::setw(8) << section_duration
              << " s"
              << " | Total elapsed: " << std::setw(8) << total_duration << " s"
              << std::endl;
    g_last_time = now;
  }
}

void __heir_debug(CryptoContextT cc, PrivateKeyT sk,
                  std::vector<CiphertextT> cts,
                  const std::map<std::string, std::string>& debugAttrMap) {
  if (!cts.empty()) {
    __heir_debug(cc, sk, cts[0], debugAttrMap);
  }
}
