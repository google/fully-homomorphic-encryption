#ifndef THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_COMMON_TRANSPILER_H_
#define THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_COMMON_TRANSPILER_H_

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/optional.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

std::string FunctionSignature(const xlscc_metadata::MetadataOutput& metadata,
                              const absl::string_view element_type,
                              absl::optional<absl::string_view> key_param_type,
                              absl::string_view key_param_name = "bk");

std::string PathToHeaderGuard(std::string_view default_value,
                              std::string_view header_path);

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_COMMON_TRANSPILER_H_
