#include "transpiler/common_transpiler.h"

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

std::string FunctionSignature(const xlscc_metadata::MetadataOutput& metadata,
                              const absl::string_view element_type,
                              absl::optional<absl::string_view> key_param_type,
                              absl::string_view key_param_name) {
  std::vector<std::string> param_signatures;
  if (!metadata.top_func_proto().return_type().has_as_void()) {
    param_signatures.push_back(
        absl::Substitute("absl::Span<$0> result", element_type));
  }
  for (auto& param : metadata.top_func_proto().params()) {
    param_signatures.push_back(
        absl::Substitute("absl::Span<$0> $1", element_type, param.name()));
  }

  if (key_param_type.has_value()) {
    param_signatures.push_back(
        absl::StrCat(key_param_type.value(), " ", key_param_name));
  }

  std::string function_name = metadata.top_func_proto().name().name();
  return absl::Substitute("absl::Status $0($1)", function_name,
                          absl::StrJoin(param_signatures, ", "));
}

std::string PathToHeaderGuard(std::string_view default_value,
                              std::string_view header_path) {
  if (header_path == "-") return std::string(default_value);
  std::string header_guard = absl::AsciiStrToUpper(header_path);
  std::transform(header_guard.begin(), header_guard.end(), header_guard.begin(),
                 [](unsigned char c) -> unsigned char {
                   if (std::isalnum(c)) {
                     return c;
                   } else {
                     return '_';
                   }
                 });
  return header_guard;
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
