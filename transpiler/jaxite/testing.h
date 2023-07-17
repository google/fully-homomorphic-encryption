#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_JAXITE_TESTING_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_JAXITE_TESTING_H_

#include <string>
#include <vector>

#include "xls/contrib/xlscc/metadata_output.pb.h"

namespace fhe {
namespace jaxite {
namespace transpiler {

inline constexpr int kParamBitWidth = 16;
inline constexpr int kReturnBitWidth = 8;

// A lightweight struct for defining parameters during test function
// construction.
struct Parameter {
  std::string name;
  bool in_out = false;
};

xlscc_metadata::MetadataOutput CreateFunctionMetadata(
    const std::vector<Parameter>& params, bool has_return_value);

}  // namespace transpiler
}  // namespace jaxite
}  // namespace fhe

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_JAXITE_TESTING_H_
