#include "transpiler/common_transpiler.h"

#include <string>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "xls/common/logging/logging.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

absl::optional<std::string> GetTypeName(
    const xlscc_metadata::InstanceType& type);

absl::optional<std::string> GetTypeName(const xlscc_metadata::Type& type) {
  if (type.has_as_bool()) {
    return "bool";
  } else if (type.has_as_int()) {
    const xlscc_metadata::IntType& int_type = type.as_int();
    switch (int_type.width()) {
      case 8:
        XLS_CHECK(int_type.has_is_declared_as_char());
        if (int_type.is_declared_as_char()) {
          return "char";
        }
        [[fallthrough]];
      case 16:
      case 32:
      case 64:
        return absl::Substitute("$0int$1_t", (int_type.is_signed() ? "" : "u"),
                                int_type.width());
    }
  } else if (type.has_as_struct()) {
    const xlscc_metadata::StructType& struct_type = type.as_struct();
    std::string name =
        struct_type.name().as_inst().name().fully_qualified_name();
    if (type.has_as_inst()) {
      const xlscc_metadata::InstanceType& inst_type = type.as_inst();
      if (inst_type.args_size()) {
        // This is a template instantiation.
        std::vector<std::string> template_args;
        for (const ::xlscc_metadata::TemplateArgument& templ :
             inst_type.args()) {
          XLS_CHECK(templ.has_as_type());
          template_args.push_back(GetTypeName(templ.as_type()).value());
        }
        name += absl::StrCat("<", absl::StrJoin(template_args, ", "), ">");
      }
    }
    return name;
  } else if (type.has_as_inst()) {
    return GetTypeName(type.as_inst());
  } else if (type.has_as_array()) {
    auto arr = type.as_array();
    return absl::StrCat(GetTypeName(arr.element_type()).value(), "[",
                        arr.size(), "]");
  }

  return absl::nullopt;
}

absl::optional<std::string> GetTypeName(
    const xlscc_metadata::InstanceType& inst_type) {
  std::string name = inst_type.name().fully_qualified_name();
  if (inst_type.args_size()) {
    // This is a template instantiation.
    std::vector<std::string> template_args;
    for (const ::xlscc_metadata::TemplateArgument& templ : inst_type.args()) {
      if (templ.has_as_type()) {
        template_args.push_back(GetTypeName(templ.as_type()).value());
      } else {
        XLS_CHECK(templ.has_as_integral());
        template_args.push_back(absl::StrCat(templ.as_integral()));
      }
    }
    name += absl::StrCat("<", absl::StrJoin(template_args, ", "), ">");
  }
  return name;
}

static std::string TypeReference(const xlscc_metadata::Type& type,
                                 bool is_reference,
                                 const absl::string_view prefix,
                                 const std::string default_type,
                                 const IdToType& id_to_type,
                                 const std::vector<std::string>& unwrap) {
  if (type.has_as_bool()) {
    return absl::Substitute("$0$1<bool>", prefix, is_reference ? "Ref" : "");
  } else if (type.has_as_int()) {
    const xlscc_metadata::IntType& int_type = type.as_int();
    switch (int_type.width()) {
      case 8:
        XLS_CHECK(int_type.has_is_declared_as_char());
        if (int_type.is_declared_as_char()) {
          return absl::Substitute("$0$1<char>", prefix,
                                  is_reference ? "Ref" : "");
        }
        return absl::Substitute("$0$2<$1 char>", prefix,
                                (int_type.is_signed() ? "signed" : "unsigned"),
                                is_reference ? "Ref" : "");
      case 16:
        return absl::Substitute("$0$2<$1 short>", prefix,
                                (int_type.is_signed() ? "signed" : "unsigned"),
                                is_reference ? "Ref" : "");
      case 32:
        return absl::Substitute("$0$2<$1 int>", prefix,
                                (int_type.is_signed() ? "signed" : "unsigned"),
                                is_reference ? "Ref" : "");
      case 64:
        return absl::Substitute("$0$2<$1 long>", prefix,
                                (int_type.is_signed() ? "signed" : "unsigned"),
                                is_reference ? "Ref" : "");
    }
  } else if (type.has_as_struct()) {
    const xlscc_metadata::StructType& struct_type = type.as_struct();
    return absl::StrCat(
        prefix, is_reference ? "Ref<" : "<",
        struct_type.name().as_inst().name().fully_qualified_name(), ">");
  } else if (type.has_as_inst()) {
    const xlscc_metadata::InstanceType& inst_type = type.as_inst();
    std::string name = GetTypeName(type).value();

    if (std::find(unwrap.begin(), unwrap.end(), name) != unwrap.end()) {
      XLS_CHECK(inst_type.name().has_id());
      auto id = inst_type.name().id();
      XLS_CHECK(id_to_type.contains(id));
      const xlscc_metadata::StructType& definition =
          id_to_type.at(inst_type.name().id()).type;
      XLS_CHECK_EQ(definition.fields_size(), 1);
      auto ref = TypeReference(definition.fields().Get(0).type(), is_reference,
                               prefix, default_type, {}, {});
      return ref;
    }
    return absl::StrCat(prefix, is_reference ? "Ref<" : "<", name, ">");
  } else if (type.has_as_array()) {
    std::vector<std::string> dimensions;
    xlscc_metadata::Type iter_type = type;
    xlscc_metadata::ArrayType array_type;
    xlscc_metadata::Type element_type;
    do {
      array_type = iter_type.as_array();
      element_type = array_type.element_type();
      iter_type = element_type;
      dimensions.push_back(absl::StrCat(array_type.size()));
    } while (element_type.has_as_array());
    std::string str_dimensions = absl::StrJoin(dimensions, ",");

    if (element_type.has_as_bool()) {
      return absl::Substitute("$0Array$1<bool,$2>", prefix,
                              is_reference ? "Ref" : "", str_dimensions);
    } else if (element_type.has_as_int()) {
      const xlscc_metadata::IntType& element_int_type = element_type.as_int();
      switch (element_int_type.width()) {
        case 8:
          XLS_CHECK(element_int_type.has_is_declared_as_char());
          // Emit a string only if the array is one-dimensional
          if (element_int_type.is_declared_as_char() &&
              dimensions.size() == 1) {
            return absl::Substitute("$0Array$1<char>", prefix,
                                    is_reference ? "Ref" : "");
          }
          [[fallthrough]];
        case 16:
        case 32:
        case 64:
          return absl::Substitute("$0Array$1<$2int$3_t,$4>", prefix,
                                  is_reference ? "Ref" : "",
                                  (element_int_type.is_signed() ? "" : "u"),
                                  element_int_type.width(), str_dimensions);
        default:
          XLS_CHECK(false);
      }
    } else {
      XLS_CHECK(element_type.has_as_inst());
      const xlscc_metadata::InstanceType& inst_type = element_type.as_inst();
      return absl::Substitute(
          "$0Array$1<$2,$3>", prefix, is_reference ? "Ref" : "",
          inst_type.name().fully_qualified_name(), str_dimensions);
    }
  }
  XLS_CHECK(false);
  return default_type;
}

static bool IsConst(const xlscc_metadata::FunctionParameter& param) {
  return !param.is_reference() || param.is_const();
}

absl::optional<std::string> TypedOverload(
    const xlscc_metadata::MetadataOutput& metadata,
    const absl::string_view prefix, const std::string default_type,
    absl::optional<absl::string_view> key_param_type,
    absl::string_view key_param_name, const std::vector<std::string>& unwrap) {
  std::vector<int64_t> struct_order = GetTypeReferenceOrder(metadata);
  const IdToType& id_to_type = PopulateTypeData(metadata, struct_order);
  std::vector<std::string> param_signatures;
  if (!metadata.top_func_proto().return_type().has_as_void()) {
    param_signatures.push_back(absl::Substitute(
        "$0 result", TypeReference(metadata.top_func_proto().return_type(),
                                   /*is_reference=*/true, prefix, default_type,
                                   id_to_type, unwrap)));
  }
  for (const xlscc_metadata::FunctionParameter& param :
       metadata.top_func_proto().params()) {
    param_signatures.push_back(absl::Substitute(
        "$0$1 $2", IsConst(param) ? "const " : "",
        TypeReference(param.type(), /*is_reference=*/true, prefix, default_type,
                      id_to_type, unwrap),
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
    if (TypeReference(param.type(), param.is_reference(), prefix, default_type,
                      id_to_type, unwrap) == default_type) {
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

// Returns the width of the given metadata type. Requires that the IdToType has
// been fully populated.
size_t GetStructWidth(const IdToType& id_to_type,
                      const xlscc_metadata::StructType& type);
size_t GetBitWidth(const IdToType& id_to_type,
                   const xlscc_metadata::Type& type) {
  if (type.has_as_void()) {
    return 0;
  } else if (type.has_as_bits()) {
    return type.as_bits().width();
  } else if (type.has_as_int()) {
    return type.as_int().width();
  } else if (type.has_as_bool()) {
    return 1;
  } else if (type.has_as_inst()) {
    int64_t type_id = type.as_inst().name().id();
    return id_to_type.at(type_id).bit_width;
  } else if (type.has_as_array()) {
    size_t element_width =
        GetBitWidth(id_to_type, type.as_array().element_type());
    return type.as_array().size() * element_width;
  }
  // Otherwise, it's a struct.
  return GetStructWidth(id_to_type, type.as_struct());
}

size_t GetStructWidth(const IdToType& id_to_type,
                      const xlscc_metadata::StructType& struct_type) {
  size_t length = 0;
  for (const xlscc_metadata::StructField& field : struct_type.fields()) {
    length += GetBitWidth(id_to_type, field.type());
  }
  return length;
}

// Gets the order in which we should process any struct definitions held in the
// given MetadataOutput.
// Since we can't assume any given ordering of output structs, we need to
// toposort.
std::vector<int64_t> GetTypeReferenceOrder(
    const xlscc_metadata::MetadataOutput& metadata) {
  using Dependees = absl::flat_hash_set<int64_t>;

  absl::flat_hash_map<int64_t, Dependees> waiters;
  std::deque<int64_t> ready;
  std::vector<int64_t> ordered_ids;
  // Initialize the ready, etc. lists.
  for (const auto& type : metadata.structs()) {
    const xlscc_metadata::StructType& struct_type = type.as_struct();
    Dependees dependees;
    for (const auto& field : struct_type.fields()) {
      // We'll never see StructTypes at anything but top level, so we only need
      // to concern ourselves with InstanceTypes.
      // Since this is initialization, we won't have seen any at this point, so
      // we can just toss them in the waiting list.
      if (field.type().has_as_inst()) {
        dependees.insert(field.type().as_inst().name().id());
      } else if (field.type().has_as_array()) {
        // Find the root of any nested arrays - what's the element type?
        const xlscc_metadata::Type* type_ref = &field.type();
        while (type_ref->has_as_array()) {
          type_ref = &type_ref->as_array().element_type();
        }

        if (type_ref->has_as_inst()) {
          dependees.insert(type_ref->as_inst().name().id());
        }
      }
    }

    // We're guaranteed that a StructType name is an InstanceType.
    int64_t struct_id = struct_type.name().as_inst().name().id();
    if (dependees.empty()) {
      ready.push_back(struct_id);
      ordered_ids.push_back(struct_id);
    } else {
      waiters[struct_id] = dependees;
    }
  }

  // Finally, walk the lists & extract the ordering.
  while (!waiters.empty()) {
    // There are types with dependencies, but at least one type in the
    // dependency chain is not included in the ready deque. This could indicate
    // a bug in the topological sort above, or else XLS did not include all the
    // structs in the metadata proto.
    if (ready.empty()) {
      XLS_LOG(FATAL) << "Dependent types missing from toposorted structs! "
                     << "Full metadata struct was:\n\n"
                     << metadata.DebugString();
    }
    // Pop the front dude off of ready.
    int64_t current_id = ready.front();
    ready.pop_front();

    std::vector<int64_t> to_remove;
    for (auto& [id, dependees] : waiters) {
      dependees.erase(current_id);
      if (dependees.empty()) {
        ready.push_back(id);
        to_remove.push_back(id);
        ordered_ids.push_back(id);
      }
    }

    for (int64_t id : to_remove) {
      waiters.erase(id);
    }
  }

  return ordered_ids;
}

// Loads an IdToType with the necessary data (type bit width, really).
IdToType PopulateTypeData(const xlscc_metadata::MetadataOutput& metadata,
                          const std::vector<int64_t>& processing_order) {
  IdToType id_to_type;
  for (int64_t id : processing_order) {
    for (const auto& type : metadata.structs()) {
      xlscc_metadata::StructType struct_type = type.as_struct();
      if (struct_type.name().as_inst().name().id() == id) {
        TypeData type_data;
        type_data.type = type.as_struct();
        type_data.bit_width = GetBitWidth(id_to_type, type);
        id_to_type[type.as_struct().name().as_inst().name().id()] = type_data;
      }
    }
  }

  return id_to_type;
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
