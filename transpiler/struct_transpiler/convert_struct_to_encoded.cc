// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "transpiler/struct_transpiler/convert_struct_to_encoded.h"

#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

using xlscc_metadata::ArrayType;
using xlscc_metadata::InstanceType;
using xlscc_metadata::IntType;
using xlscc_metadata::MetadataOutput;
using xlscc_metadata::StructField;
using xlscc_metadata::StructType;
using xlscc_metadata::Type;

// Simple holder for a type and its total bit width.
struct TypeData {
  xlscc_metadata::StructType type;
  size_t bit_width;
};

using IdToType = absl::flat_hash_map<int64_t, TypeData>;

// Returns the width of the given metadata type. Requires that the IdToType has
// been fully populated.
size_t GetStructWidth(const IdToType& id_to_type, const StructType& type);
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
                      const StructType& struct_type) {
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
    const StructType& struct_type = type.as_struct();
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
        const Type* type_ref = &field.type();
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
      StructType struct_type = type.as_struct();
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

// Trivial reverse-lookup fn.
absl::StatusOr<std::string> XlsccToNativeIntegerType(const IntType& int_type) {
  std::string base;
  switch (int_type.width()) {
    case 1:
      return "bool";
    case 8:
      base = "int8_t";
      break;
    case 16:
      base = "int16_t";
      break;
    case 32:
      base = "int32_t";
      break;
    case 64:
      base = "int64_t";
      break;
    default:
      return absl::InvalidArgumentError(
          absl::StrCat("Unknown integer width: ", int_type.width()));
  }
  return absl::StrCat(int_type.is_signed() ? "" : "u", base);
}

absl::StatusOr<std::string> GenerateSetOrEncryptArrayElement(
    const IdToType& id_to_type, absl::string_view element_name,
    const ArrayType& array_type, bool encrypt, int loop_nest = 0);
absl::StatusOr<std::string> GenerateSetOrEncryptBoolElement(
    absl::string_view source, bool encrypt);
absl::StatusOr<std::string> GenerateSetOrEncryptIntegralElement(
    const Type& type, absl::string_view source_var, bool encrypt);
absl::StatusOr<std::string> GenerateSetOrEncryptStructElement(
    const IdToType& id_to_type, const InstanceType& instance_type,
    absl::string_view source_var, bool encrypt);

absl::StatusOr<std::string> GenerateSetOrEncryptOneElement(
    const IdToType& id_to_type, const StructField& field, bool encrypt) {
  std::vector<std::string> lines;
  if (field.type().has_as_array()) {
    XLS_ASSIGN_OR_RETURN(
        std::string array_lines,
        GenerateSetOrEncryptArrayElement(id_to_type, field.name(),
                                         field.type().as_array(), encrypt));
    lines.push_back(array_lines);
  } else if (field.type().has_as_bool()) {
    XLS_ASSIGN_OR_RETURN(
        std::string bool_lines,
        GenerateSetOrEncryptBoolElement(field.name(), encrypt));
    lines.push_back(bool_lines);
  } else if (field.type().has_as_int()) {
    XLS_ASSIGN_OR_RETURN(std::string int_lines,
                         GenerateSetOrEncryptIntegralElement(
                             field.type(), field.name(), encrypt));
    lines.push_back(int_lines);
  } else if (field.type().has_as_inst()) {
    XLS_ASSIGN_OR_RETURN(
        std::string struct_lines,
        GenerateSetOrEncryptStructElement(id_to_type, field.type().as_inst(),
                                          field.name(), encrypt));
    lines.push_back(struct_lines);
  } else {
    return absl::InvalidArgumentError(
        absl::StrCat("Unknown/unsupported struct elements type: ",
                     field.type().ShortDebugString()));
  }

  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateSetOrEncryptArrayElement(
    const IdToType& id_to_type, absl::string_view element_name,
    const ArrayType& array_type, bool encrypt, int loop_nest) {
  const Type& sub_element_type = array_type.element_type();
  std::string index_var = absl::StrCat("idx_", loop_nest);
  std::string index_expr;
  for (int i = 0; i <= loop_nest; i++) {
    absl::StrAppend(&index_expr, "[idx_", i, "]");
  }

  std::vector<std::string> lines;
  lines.push_back(absl::Substitute("    for (int $0 = 0; $0 < $1; $0++) {",
                                   index_var, array_type.size()));
  if (sub_element_type.has_as_array()) {
    XLS_ASSIGN_OR_RETURN(std::string foo, GenerateSetOrEncryptArrayElement(
                                              id_to_type, element_name,
                                              sub_element_type.as_array(),
                                              encrypt, loop_nest + 1));
    lines.push_back(foo);
  } else if (sub_element_type.has_as_bits()) {
    XLS_ASSIGN_OR_RETURN(std::string foo,
                         GenerateSetOrEncryptBoolElement(
                             absl::StrCat(element_name, index_expr), encrypt));
    lines.push_back(foo);
  } else if (sub_element_type.has_as_int()) {
    XLS_ASSIGN_OR_RETURN(
        std::string foo,
        GenerateSetOrEncryptIntegralElement(
            sub_element_type, absl::StrCat(element_name, index_expr), encrypt));
    lines.push_back(foo);
  } else if (sub_element_type.has_as_inst()) {
    XLS_ASSIGN_OR_RETURN(std::string foo,
                         GenerateSetOrEncryptStructElement(
                             id_to_type, sub_element_type.as_inst(),
                             absl::StrCat(element_name, index_expr), encrypt));
    lines.push_back(foo);
  }
  lines.push_back("    }");

  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateSetOrEncryptBoolElement(
    absl::string_view source, bool encrypt) {
  std::vector<std::string> lines;
  std::string op = encrypt ? "Encrypt" : "Unencrypted";
  lines.push_back(absl::Substitute(
      "    ::$0(EncodedValue<bool>(value.$1), key, data);", op, source));
  lines.push_back("    data += 1;");
  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateSetOrEncryptIntegralElement(
    const Type& type, absl::string_view source_var, bool encrypt) {
  std::vector<std::string> lines;
  std::string op = encrypt ? "Encrypt" : "Unencrypted";
  IntType int_type = type.as_int();
  XLS_ASSIGN_OR_RETURN(std::string int_type_name,
                       XlsccToNativeIntegerType(int_type));

  lines.push_back(
      absl::Substitute("    ::$0(EncodedValue<$1>(value.$2), key, data);", op,
                       int_type_name, source_var));
  lines.push_back(absl::StrCat("    data += ", int_type.width(), ";"));

  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateSetOrEncryptStructElement(
    const IdToType& id_to_type, const InstanceType& instance_type,
    absl::string_view source_var, bool encrypt) {
  std::vector<std::string> lines;
  std::string op = encrypt ? "SetEncrypted" : "SetUnencrypted";

  TypeData type_data = id_to_type.at(instance_type.name().id());
  std::string struct_name = absl::StrCat("Fhe", instance_type.name().name());
  lines.push_back(absl::Substitute("    $2::Borrowed$0(value.$1, key, data);",
                                   op, source_var, struct_name));
  lines.push_back(absl::StrCat("    data += ", type_data.bit_width, ";"));

  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateSetOrEncryptFunction(
    const IdToType& id_to_type, const StructType& struct_type, bool encrypt) {
  std::vector<std::string> lines;
  for (int i = 0; i < struct_type.fields_size(); i++) {
    XLS_ASSIGN_OR_RETURN(std::string line,
                         GenerateSetOrEncryptOneElement(
                             id_to_type, struct_type.fields(i), encrypt));
    lines.push_back(line);
  }
  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateDecryptArray(const IdToType& id_to_type,
                                                 const ArrayType& array_type,
                                                 absl::string_view output_loc,
                                                 int loop_nest = 0);
absl::StatusOr<std::string> GenerateDecryptBool(const IdToType& id_to_type,
                                                const std::string& temp_name,
                                                absl::string_view output_loc);
absl::StatusOr<std::string> GenerateDecryptIntegral(
    const IdToType& id_to_type, const IntType& int_type,
    const std::string& temp_name, absl::string_view output_loc);
absl::StatusOr<std::string> GenerateDecryptStruct(
    const IdToType& id_to_type, const InstanceType& instance_type,
    absl::string_view output_loc);

absl::StatusOr<std::string> GenerateDecryptOneElement(
    const IdToType& id_to_type, const StructField& field,
    absl::string_view output_loc) {
  std::vector<std::string> lines;
  if (field.type().has_as_array()) {
    return GenerateDecryptArray(id_to_type, field.type().as_array(),
                                output_loc);
  } else if (field.type().has_as_bool()) {
    return GenerateDecryptBool(id_to_type, field.name(), output_loc);
  } else if (field.type().has_as_int()) {
    return GenerateDecryptIntegral(id_to_type, field.type().as_int(),
                                   field.name(), output_loc);
  } else if (field.type().has_as_inst()) {
    return GenerateDecryptStruct(id_to_type, field.type().as_inst(),
                                 output_loc);
  } else {
    return absl::InvalidArgumentError(
        absl::StrCat("Unknown/unsupported struct field type: ",
                     field.type().ShortDebugString()));
  }

  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateDecryptArray(const IdToType& id_to_type,
                                                 const ArrayType& array_type,
                                                 absl::string_view output_loc,
                                                 int loop_nest) {
  const Type& element_type = array_type.element_type();
  std::string index_var = absl::StrCat("idx_", loop_nest);
  std::string index_expr = absl::StrCat(output_loc, "[", index_var, "]");
  std::string var_name = absl::StrCat("tmp_", loop_nest);

  std::vector<std::string> lines;
  lines.push_back(absl::Substitute("    for (int $0 = 0; $0 < $1; $0++) {",
                                   index_var, array_type.size()));
  if (element_type.has_as_array()) {
    XLS_ASSIGN_OR_RETURN(
        std::string foo,
        GenerateDecryptArray(id_to_type, element_type.as_array(), index_expr,
                             loop_nest + 1));
    lines.push_back(foo);
  }
  if (element_type.has_as_bits()) {
    XLS_ASSIGN_OR_RETURN(std::string foo,
                         GenerateDecryptBool(id_to_type, var_name, index_expr));
    lines.push_back(foo);
  } else if (element_type.has_as_int()) {
    XLS_ASSIGN_OR_RETURN(std::string foo, GenerateDecryptIntegral(
                                              id_to_type, element_type.as_int(),
                                              var_name, index_expr));
    lines.push_back(foo);
  } else if (element_type.has_as_inst()) {
    XLS_ASSIGN_OR_RETURN(
        std::string foo,
        GenerateDecryptStruct(id_to_type, element_type.as_inst(), index_expr));
    lines.push_back(foo);
  }
  lines.push_back("    }");

  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateDecryptBool(const IdToType& id_to_type,
                                                const std::string& temp_name,
                                                absl::string_view output_loc) {
  std::vector<std::string> lines;
  lines.push_back(
      absl::StrCat("    EncodedValue<bool> encoded_", temp_name, ";"));
  lines.push_back(
      absl::StrCat("    ::Decrypt(data, key, encoded_", temp_name, ");"));
  lines.push_back("    data += 1;");
  lines.push_back(absl::Substitute("    result->$0 = encoded_$1.Decode();",
                                   output_loc, temp_name));
  return absl::StrJoin(lines, "\n");
}
absl::StatusOr<std::string> GenerateDecryptIntegral(
    const IdToType& id_to_type, const IntType& int_type,
    const std::string& temp_name, absl::string_view output_loc) {
  std::vector<std::string> lines;
  XLS_ASSIGN_OR_RETURN(std::string int_type_name,
                       XlsccToNativeIntegerType(int_type));
  lines.push_back(absl::Substitute("    EncodedValue<$0> encoded_$1;",
                                   int_type_name, temp_name));
  lines.push_back(
      absl::Substitute("    ::Decrypt(data, key, encoded_$0);", temp_name));
  lines.push_back(absl::StrCat("    data += ", int_type.width(), ";"));
  lines.push_back(absl::Substitute("    result->$0 = encoded_$1.Decode();",
                                   output_loc, temp_name));
  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateDecryptStruct(
    const IdToType& id_to_type, const InstanceType& instance_type,
    absl::string_view output_loc) {
  std::vector<std::string> lines;
  TypeData type_data = id_to_type.at(instance_type.name().id());
  std::string struct_name = absl::StrCat("Fhe", instance_type.name().name());
  lines.push_back(
      absl::Substitute("    $0::BorrowedDecrypt(key, data, &result->$1);",
                       struct_name, output_loc));
  lines.push_back(absl::StrCat("    data += ", type_data.bit_width, ";"));
  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateDecryptFunction(
    const IdToType& id_to_type, const StructType& struct_type) {
  std::vector<std::string> lines;
  for (int i = 0; i < struct_type.fields_size(); i++) {
    const StructField& field = struct_type.fields(i);
    XLS_ASSIGN_OR_RETURN(std::string line,
                         GenerateDecryptOneElement(
                             id_to_type, field, struct_type.fields(i).name()));
    lines.push_back(line);
  }

  return absl::StrJoin(lines, "\n");
}

// The big honkin' header file template. Substitution params:
//  0: The header from which we're transpiling. The root struct source.
//  1: The name of the struct we're transpiling.
//  2: The fully-qualified name of the struct we're transpiling.
//  3: The body of the internal "Set" routine.
//  4: The body of the internal "Encrypt" routine.
//  5: The body of the internal "Decrypt" routine.
//  6: The [packed] bit width of the structure.
//
// TODO: Add header guards.
constexpr const char kFileTemplate[] = R"(
#include <cstdint>
#include <memory>

$0

#include "tfhe/tfhe.h"
#include "transpiler/data/fhe_data.h"

$1
)";

constexpr const char kClassTemplate[] = R"(
class Fhe$0 {
 public:
  Fhe$0(const TFheGateBootstrappingParameterSet* params)
      : data_(new_gate_bootstrapping_ciphertext_array(bit_width_, params),
              LweSampleArrayDeleter(bit_width_)) { }

  // We set values here directly, instead of using FheValue, since FheValue
  // types own their arrays, whereas we'd need to own them here as
  // contiguously-allocated chunks. (We could modify them to use borrowed data,
  // but it'd be more work than this). For structure types, though, we do
  // enable setting on "borrowed" data to enable recursive setting.
  void SetUnencrypted(const $1& value,
                      const TFheGateBootstrappingCloudKeySet* key) {
    SetUnencryptedInternal(value, key, data_.get());
  }

  void SetEncrypted(const $1& value,
                    const TFheGateBootstrappingSecretKeySet* key) {
    SetEncryptedInternal(value, key, data_.get());
  }

  $1 Decrypt(const TFheGateBootstrappingSecretKeySet* key) {
    $1 result;
    DecryptInternal(key, data_.get(), &result);
    return result;
  }

  static void BorrowedSetUnencrypted(
      const $1& value, const TFheGateBootstrappingCloudKeySet* key,
      LweSample* data) {
    SetUnencryptedInternal(value, key, data);
  }

  static void BorrowedSetEncrypted(
      const $1& value, const TFheGateBootstrappingSecretKeySet* key,
      LweSample* data) {
    SetEncryptedInternal(value, key, data);
  }

  static void BorrowedDecrypt(
      const TFheGateBootstrappingSecretKeySet* key,
      LweSample* data, $1* result) {
    DecryptInternal(key, data, result);
  }

  LweSample* get() { return data_.get(); }
  size_t bit_width() { return bit_width_; }

 private:
  size_t bit_width_ = $5;

  static void SetUnencryptedInternal(
      const $1& value, const TFheGateBootstrappingCloudKeySet* key,
      LweSample* data) {
$2
  }

  static void SetEncryptedInternal(
      const $1& value, const TFheGateBootstrappingSecretKeySet* key,
      LweSample* data) {
$3
  }

  static void DecryptInternal(
      const TFheGateBootstrappingSecretKeySet* key, LweSample* data,
      $1* result) {
$4
  }

  std::unique_ptr<LweSample, LweSampleArrayDeleter> data_;
};
)";

absl::StatusOr<std::string> ConvertStructToEncoded(const IdToType& id_to_type,
                                                   int64_t id) {
  const StructType& struct_type = id_to_type.at(id).type;

  int64_t bit_width = GetStructWidth(id_to_type, struct_type);

  xlscc_metadata::CPPName struct_name = struct_type.name().as_inst().name();
  XLS_ASSIGN_OR_RETURN(std::string set_fn,
                       GenerateSetOrEncryptFunction(id_to_type, struct_type,
                                                    /*encrypt=*/false));
  XLS_ASSIGN_OR_RETURN(std::string encrypt_fn,
                       GenerateSetOrEncryptFunction(id_to_type, struct_type,
                                                    /*encrypt=*/true));
  XLS_ASSIGN_OR_RETURN(std::string decrypt_fn,
                       GenerateDecryptFunction(id_to_type, struct_type));
  std::string result = absl::Substitute(
      kClassTemplate, struct_name.name(), struct_name.fully_qualified_name(),
      set_fn, encrypt_fn, decrypt_fn, bit_width);
  return result;
}

}  // namespace

absl::StatusOr<std::string> ConvertStructsToEncoded(
    const xlscc_metadata::MetadataOutput& metadata,
    const std::vector<std::string>& original_headers) {
  if (metadata.structs_size() == 0) {
    return "";
  }

  std::vector<int64_t> struct_order = GetTypeReferenceOrder(metadata);
  IdToType id_to_type = PopulateTypeData(metadata, struct_order);
  std::vector<std::string> generated;
  for (int64_t id : struct_order) {
    XLS_ASSIGN_OR_RETURN(std::string struct_text,
                         ConvertStructToEncoded(id_to_type, id));
    generated.push_back(struct_text);
  }

  std::vector<std::string> original_includes;
  original_includes.reserve(original_headers.size());
  for (const auto& header : original_headers) {
    original_includes.push_back(absl::StrCat("#include \"", header, "\""));
  }
  std::string extra_includes = absl::StrJoin(original_includes, "\n");

  return absl::Substitute(kFileTemplate, extra_includes,
                          absl::StrJoin(generated, "\n\n"));
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
