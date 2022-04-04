#include "transpiler/common_transpiler.h"

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "xls/common/logging/logging.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

static std::string TypeReference(const xlscc_metadata::Type& type,
                                 bool is_reference,
                                 const absl::string_view prefix,
                                 const std::string default_type) {
  if (type.has_as_bool()) {
    return absl::Substitute("$0Value$1<bool>", prefix,
                            is_reference ? "Ref" : "");
  } else if (type.has_as_int()) {
    const xlscc_metadata::IntType& int_type = type.as_int();
    switch (int_type.width()) {
      case 8:
        XLS_CHECK(int_type.has_is_declared_as_char());
        if (int_type.is_declared_as_char()) {
          return absl::Substitute("$0Char$1", prefix,
                                  is_reference ? "Ref" : "");
        }
        [[fallthrough]];
      case 16:
      case 32:
      case 64:
        return absl::Substitute(
            "$0Value$1<$2int$3_t>", prefix, is_reference ? "Ref" : "",
            (int_type.is_signed() ? "" : "u"), int_type.width());
    }
  } else if (type.has_as_array()) {
    const xlscc_metadata::ArrayType& array_type = type.as_array();
    const xlscc_metadata::Type& element_type = array_type.element_type();

    if (element_type.has_as_bool()) {
      return absl::Substitute("$0Array$1<bool>", prefix,
                              is_reference ? "Ref" : "");
    } else if (element_type.has_as_int()) {
      const xlscc_metadata::IntType& element_int_type = element_type.as_int();
      switch (element_int_type.width()) {
        case 8:
          XLS_CHECK(element_int_type.has_is_declared_as_char());
          if (element_int_type.is_declared_as_char()) {
            return absl::Substitute("$0String$1", prefix,
                                    is_reference ? "Ref" : "");
          }
          [[fallthrough]];
        case 16:
        case 32:
        case 64:
          return absl::Substitute("$0Array$1<$2int$3_t>", prefix,
                                  is_reference ? "Ref" : "",
                                  (element_int_type.is_signed() ? "" : "u"),
                                  element_int_type.width());
      }
    }
  } else if (type.has_as_struct()) {
    const xlscc_metadata::StructType& struct_type = type.as_struct();
    return absl::StrCat(prefix, struct_type.name().as_inst().name().name(),
                        is_reference ? "Ref" : "");
  } else if (type.has_as_inst()) {
    const xlscc_metadata::InstanceType& inst_type = type.as_inst();
    return absl::StrCat(prefix, inst_type.name().name(),
                        is_reference ? "Ref" : "");
  }
  return default_type;
}

static bool IsConst(const xlscc_metadata::FunctionParameter& param) {
  return !param.is_reference() || param.is_const();
}

absl::optional<std::string> TypedOverload(
    const xlscc_metadata::MetadataOutput& metadata,
    const absl::string_view prefix, const std::string default_type,
    absl::optional<absl::string_view> key_param_type,
    absl::string_view key_param_name) {
  std::vector<std::string> param_signatures;
  if (!metadata.top_func_proto().return_type().has_as_void()) {
    param_signatures.push_back(absl::Substitute(
        "$0 result",
        TypeReference(metadata.top_func_proto().return_type(),
                      /*is_reference=*/true, prefix, default_type)));
  }
  for (const xlscc_metadata::FunctionParameter& param :
       metadata.top_func_proto().params()) {
    param_signatures.push_back(
        absl::Substitute("$0$1 $2", IsConst(param) ? "const " : "",
                         TypeReference(param.type(), /*is_reference=*/true,
                                       prefix, default_type),
                         param.name()));
  }

  std::string function_name = metadata.top_func_proto().name().name();
  std::string prototype;

  if (param_signatures.empty()) {
    prototype = absl::Substitute(
        "absl::Status $0($1)", function_name,
        key_param_type.has_value()
            ? absl::StrCat(key_param_type.value(), " ", key_param_name)
            : "");
  } else {
    prototype = absl::Substitute(
        "absl::Status $0($1$2)", function_name,
        absl::StrJoin(param_signatures, ", "),
        key_param_type.has_value()
            ? absl::StrCat(",\n ", key_param_type.value(), " ", key_param_name)
            : "");
  }

  std::vector<std::string> param_refs;
  if (!metadata.top_func_proto().return_type().has_as_void()) {
    param_refs.push_back("result.get()");
  }
  for (const xlscc_metadata::FunctionParameter& param :
       metadata.top_func_proto().params()) {
    if (TypeReference(param.type(), param.is_reference(), prefix,
                      default_type) == default_type) {
      param_refs.push_back(param.name());
    } else {
      param_refs.push_back(absl::StrCat(param.name(), ".get()"));
    }
  }
  if (key_param_type.has_value()) {
    param_refs.push_back(std::string(key_param_name));
  }

  constexpr absl::string_view overload_template = R"($0 {
  return $1_UNSAFE($2);
}
)";
  return absl::Substitute(overload_template, prototype, function_name,
                          absl::StrJoin(param_refs, ", "));
}

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
    param_signatures.push_back(absl::Substitute("absl::Span<$0$1> $2",
                                                IsConst(param) ? "const " : "",
                                                element_type, param.name()));
  }

  if (key_param_type.has_value()) {
    param_signatures.push_back(
        absl::StrCat(key_param_type.value(), " ", key_param_name));
  }

  std::string function_name = metadata.top_func_proto().name().name();
  return absl::Substitute("absl::Status $0_UNSAFE($1)", function_name,
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
