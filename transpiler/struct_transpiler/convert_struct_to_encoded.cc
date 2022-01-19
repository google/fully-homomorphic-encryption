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

// TODO: Fix get() method to not pass mutable pointer from data_, or stop
// pretending that data_ is a std::unique_ptr.

#include "transpiler/struct_transpiler/convert_struct_to_encoded.h"

#include <cctype>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
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
  std::string op =
      encrypt ? "Encrypt<Sample,SecretKey>" : "Unencrypted<Sample,PublicKey>";
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
  std::string struct_name =
      absl::StrCat("GenericEncoded", instance_type.name().name());
  lines.push_back(
      absl::Substitute("    $2<Sample, SampleArrayDeleter, SecretKey, "
                       "PublicKey>::Borrowed$0(value.$1, data, key);",
                       op, source_var, struct_name));
  lines.push_back(absl::StrCat("    data += ", type_data.bit_width, ";"));

  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateSetOrEncryptFunction(
    const IdToType& id_to_type, const StructType& struct_type, bool encrypt,
    bool reverse) {
  std::vector<std::string> lines;
  for (int i = 0; i < struct_type.fields_size(); i++) {
    XLS_ASSIGN_OR_RETURN(std::string line,
                         GenerateSetOrEncryptOneElement(
                             id_to_type, struct_type.fields(i), encrypt));
    lines.push_back(line);
  }
  if (reverse) {
    std::reverse(lines.begin(), lines.end());
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
  lines.push_back(absl::StrCat(
      "    ::Decrypt<Sample, SecretKey>(data, key, encoded_", temp_name, ");"));
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
  lines.push_back(absl::Substitute(
      "    ::Decrypt<Sample, SecretKey>(data, key, encoded_$0);", temp_name));
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
  std::string struct_name =
      absl::StrCat("GenericEncoded", instance_type.name().name());
  lines.push_back(
      absl::Substitute("    $0<Sample, SampleArrayDeleter, SecretKey, "
                       "PublicKey>::BorrowedDecrypt(data, &result->$1, key);",
                       struct_name, output_loc));
  lines.push_back(absl::StrCat("    data += ", type_data.bit_width, ";"));
  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateDecryptFunction(
    const IdToType& id_to_type, const StructType& struct_type, bool reverse) {
  std::vector<std::string> lines;
  for (int i = 0; i < struct_type.fields_size(); i++) {
    const StructField& field = struct_type.fields(i);
    XLS_ASSIGN_OR_RETURN(std::string line,
                         GenerateDecryptOneElement(
                             id_to_type, field, struct_type.fields(i).name()));
    lines.push_back(line);
  }
  if (reverse) {
    std::reverse(lines.begin(), lines.end());
  }

  return absl::StrJoin(lines, "\n");
}

static std::string GenerateHeaderGuard(absl::string_view header) {
  std::string header_guard = absl::AsciiStrToUpper(header);
  std::transform(header_guard.begin(), header_guard.end(), header_guard.begin(),
                 [](unsigned char c) -> unsigned char {
                   if (std::isalnum(c)) {
                     return c;
                   } else {
                     return '_';
                   }
                 });
  return absl::StrCat("GENERATED_", header_guard);
}

// The big honkin' header file template. Substitution params:
//  0: The header from which we're transpiling. The root struct source.
//  1: The name of the struct we're transpiling.
//  2: The header guard.
constexpr const char kFileTemplate[] = R"(#ifndef $2
#define $2

#include <cstdint>
#include <memory>

#include "absl/types/span.h"

$0
#include "transpiler/data/boolean_data.h"

template <class Sample, class PublicKey>
void Unencrypted(absl::Span<const bool> value, const PublicKey* key, Sample* out);

template <class Sample, class SecretKey>
void Encrypt(absl::Span<const bool> value, const SecretKey* key, Sample* out);

template <class Sample, class SecretKey>
void Decrypt(Sample* ciphertext, const SecretKey* key, absl::Span<bool> plaintext);

template <class Sample, class SampleArrayDeleter>
using SampleArrayT = std::unique_ptr<Sample[], SampleArrayDeleter>;

$1
#endif//$2)";

// The template for each individual struct. Substitution params:
//  0: The header from which we're transpiling. The root struct source.
//  1: The name of the struct we're transpiling.
//  2: The fully-qualified name of the struct we're transpiling.
//  3: The body of the internal "Set" routine.
//  4: The body of the internal "Encrypt" routine.
//  5: The body of the internal "Decrypt" routine.
//  6: The [packed] bit width of the structure.
constexpr const char kClassTemplate[] = R"(
template <class Sample, class SampleArrayDeleter, class SecretKey, class PublicKey>
class GenericEncoded$0 {
 public:
  GenericEncoded$0(Sample *data, SampleArrayDeleter deleter)
      : data_(data, deleter) {}
  GenericEncoded$0(GenericEncoded$0 &&) = default;

  // We set values here directly, instead of using TfheValue, since TfheValue
  // types own their arrays, whereas we'd need to own them here as
  // contiguously-allocated chunks. (We could modify them to use borrowed data,
  // but it'd be more work than this). For structure types, though, we do
  // enable setting on "borrowed" data to enable recursive setting.
  void SetUnencrypted(const $1& value,
                      const PublicKey* key) {
    SetUnencryptedInternal(value, key, data_.get());
  }

  void SetEncrypted(const $1& value,
                    const SecretKey* key) {
    SetEncryptedInternal(value, key, data_.get());
  }

  $1 Decrypt(const SecretKey* key) {
    $1 result;
    DecryptInternal(key, data_.get(), &result);
    return result;
  }

  static void BorrowedSetUnencrypted(
      const $1& value, Sample* data, const PublicKey* key) {
    SetUnencryptedInternal(value, key, data);
  }

  static void BorrowedSetEncrypted(
      const $1& value, Sample* data, const SecretKey* key) {
    SetEncryptedInternal(value, key, data);
  }

  static void BorrowedDecrypt(
      Sample* data, $1* result, const SecretKey* key) {
    DecryptInternal(key, data, result);
  }

  absl::Span<Sample> get() { return absl::MakeSpan(data_.get(), bit_width()); }
  static constexpr size_t bit_width() { return bit_width_; }

 private:
  static constexpr const size_t bit_width_ = $5;

  static void SetUnencryptedInternal(
      const $1& value, const PublicKey* key,
      Sample* data) {
$2
  }

  static void SetEncryptedInternal(
      const $1& value, const SecretKey* key,
      Sample* data) {
$3
  }

  static void DecryptInternal(
      const SecretKey* key, Sample* data,
      $1* result) {
$4
  }

  SampleArrayT<Sample, SampleArrayDeleter> data_;
};)";

absl::StatusOr<std::string> ConvertStructToEncoded(const IdToType& id_to_type,
                                                   int64_t id, bool reverse) {
  const StructType& struct_type = id_to_type.at(id).type;

  int64_t bit_width = GetStructWidth(id_to_type, struct_type);

  xlscc_metadata::CPPName struct_name = struct_type.name().as_inst().name();
  XLS_ASSIGN_OR_RETURN(std::string set_fn, GenerateSetOrEncryptFunction(
                                               id_to_type, struct_type,
                                               /*encrypt=*/false, reverse));
  XLS_ASSIGN_OR_RETURN(std::string encrypt_fn,
                       GenerateSetOrEncryptFunction(id_to_type, struct_type,
                                                    /*encrypt=*/true, reverse));
  XLS_ASSIGN_OR_RETURN(
      std::string decrypt_fn,
      GenerateDecryptFunction(id_to_type, struct_type, reverse));
  std::string result = absl::Substitute(
      kClassTemplate, struct_name.name(), struct_name.fully_qualified_name(),
      set_fn, encrypt_fn, decrypt_fn, bit_width);
  return result;
}

}  // namespace

absl::StatusOr<std::string> ConvertStructsToEncodedTemplate(
    const xlscc_metadata::MetadataOutput& metadata,
    const std::vector<std::string>& original_headers,
    absl::string_view output_path, bool struct_fields_in_declaration_order) {
  if (metadata.structs_size() == 0) {
    return "";
  }

  std::string header_guard = GenerateHeaderGuard(output_path);

  bool reverse = struct_fields_in_declaration_order;

  std::vector<int64_t> struct_order = GetTypeReferenceOrder(metadata);
  IdToType id_to_type = PopulateTypeData(metadata, struct_order);
  std::vector<std::string> generated;
  for (int64_t id : struct_order) {
    XLS_ASSIGN_OR_RETURN(std::string struct_text,
                         ConvertStructToEncoded(id_to_type, id, reverse));
    generated.push_back(struct_text);
  }

  std::vector<std::string> original_includes;
  original_includes.reserve(original_headers.size());
  for (const auto& header : original_headers) {
    original_includes.push_back(absl::StrCat("#include \"", header, "\""));
  }
  std::string extra_includes = absl::StrJoin(original_includes, "\n");

  return absl::Substitute(kFileTemplate, extra_includes,
                          absl::StrJoin(generated, "\n\n"), header_guard);
}

// Bool header template
//  0: Header guard
//  1: Generic header
//  2: Class definitions
constexpr const char kBoolFileTemplate[] = R"(#ifndef $0
#define $0

#include <memory>

#include "$1"
#include "absl/types/span.h"

struct DummyPublicKey {};
struct DummySecretKey {};

template<>
void Unencrypted<bool, DummyPublicKey>(absl::Span<const bool> value, const DummyPublicKey* key, bool* out) {
  for (int j = 0; j < value.size(); ++j) {
    out[j] = value[j];
  }
}

template<>
void Encrypt<bool, DummySecretKey>(absl::Span<const bool> value, const DummySecretKey* key, bool* out) {
  for (int j = 0; j < value.size(); ++j) {
    out[j] = value[j];
  }
}

template<>
void Decrypt<bool, DummySecretKey>(bool* ciphertext, const DummySecretKey* key, absl::Span<bool> plaintext) {
  for (int j = 0; j < plaintext.size(); j++) {
    plaintext[j] = ciphertext[j];
  }
}

$2
#endif//$0)";

// Bool struct template
//  0: Header guard
//  1: Type name
constexpr const char kBoolStructTemplate[] = R"(
using EncodedBase$0 = GenericEncoded$0<bool, std::default_delete<bool[]>, DummySecretKey, DummyPublicKey>;
class Encoded$0 : public EncodedBase$0 {
public:
  Encoded$0() :
      EncodedBase$0(new bool[Encoded$0::bit_width()],
                 std::default_delete<bool[]>()) {}

  void Encode(const $0& value) {
      SetEncrypted(value, nullptr);
  }

  $0 Decode() {
      return Decrypt(nullptr);
  }

private:
  using EncodedBase$0::Decrypt;
  using EncodedBase$0::SetUnencrypted;
  using EncodedBase$0::SetEncrypted;
  using EncodedBase$0::BorrowedSetUnencrypted;
  using EncodedBase$0::BorrowedSetEncrypted;
  using EncodedBase$0::BorrowedDecrypt;
};

)";

absl::StatusOr<std::string> ConvertStructsToEncodedBool(
    absl::string_view generic_header,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view output_path) {
  if (metadata.structs_size() == 0) {
    return "";
  }
  std::string header_guard = GenerateHeaderGuard(output_path);
  std::vector<int64_t> struct_order = GetTypeReferenceOrder(metadata);
  IdToType id_to_type = PopulateTypeData(metadata, struct_order);
  std::vector<std::string> generated;
  for (int64_t id : struct_order) {
    const StructType& struct_type = id_to_type.at(id).type;
    xlscc_metadata::CPPName struct_name = struct_type.name().as_inst().name();
    std::string struct_text =
        absl::Substitute(kBoolStructTemplate, struct_name.name());
    generated.push_back(struct_text);
  }

  return absl::Substitute(kBoolFileTemplate, header_guard, generic_header,
                          absl::StrJoin(generated, "\n\n"));
}

// Tfhe header template
//  0: Header guard
//  1: Generic header
//  2: Class definitions
constexpr const char kTfheFileTemplate[] = R"(#ifndef $0
#define $0

#include <memory>

#include "$1"
#include "absl/types/span.h"
#include "transpiler/data/tfhe_data.h"
#include "tfhe/tfhe.h"

template<>
void Unencrypted<LweSample, TFheGateBootstrappingCloudKeySet>(absl::Span<const bool> value, const TFheGateBootstrappingCloudKeySet* key, LweSample* out) {
  ::Unencrypted(value, key, out);
}

template<>
void Encrypt<LweSample, TFheGateBootstrappingSecretKeySet>(absl::Span<const bool> value, const TFheGateBootstrappingSecretKeySet* key, LweSample* out) {
  ::Encrypt(value, key, out);
}

template<>
void Decrypt<LweSample, TFheGateBootstrappingSecretKeySet>(LweSample* ciphertext, const TFheGateBootstrappingSecretKeySet* key, absl::Span<bool> plaintext) {
  ::Decrypt(ciphertext, key, plaintext);
}

$2
#endif//$0)";

// Tfhe struct template
//  0: Header guard
//  1: Type name
constexpr const char kTfheStructTemplate[] = R"(
using TfheBase$0 = GenericEncoded$0<LweSample, LweSampleArrayDeleter, TFheGateBootstrappingSecretKeySet, TFheGateBootstrappingCloudKeySet>;
class Tfhe$0 : public TfheBase$0 {
public:
  Tfhe$0(const TFheGateBootstrappingParameterSet* params) :
      TfheBase$0(new_gate_bootstrapping_ciphertext_array(Tfhe$0::bit_width(), params),
                 LweSampleArrayDeleter(Tfhe$0::bit_width())) {}
};)";

absl::StatusOr<std::string> ConvertStructsToEncodedTfhe(
    absl::string_view generic_header,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view output_path) {
  if (metadata.structs_size() == 0) {
    return "";
  }
  std::string header_guard = GenerateHeaderGuard(output_path);
  std::vector<int64_t> struct_order = GetTypeReferenceOrder(metadata);
  IdToType id_to_type = PopulateTypeData(metadata, struct_order);
  std::vector<std::string> generated;
  for (int64_t id : struct_order) {
    const StructType& struct_type = id_to_type.at(id).type;
    xlscc_metadata::CPPName struct_name = struct_type.name().as_inst().name();
    std::string struct_text =
        absl::Substitute(kTfheStructTemplate, struct_name.name());
    generated.push_back(struct_text);
  }

  return absl::Substitute(kTfheFileTemplate, header_guard, generic_header,
                          absl::StrJoin(generated, "\n\n"));
}

// PALISADE header template
//  0: Header guard
//  1: Generic header
//  2: Class definitions
constexpr const char kPalisadeFileTemplate[] = R"(#ifndef $0
#define $0

#include <memory>

#include "$1"
#include "absl/types/span.h"
#include "transpiler/data/palisade_data.h"
#include "palisade/binfhe/binfhecontext.h"

template<>
void Unencrypted<lbcrypto::LWECiphertext, lbcrypto::BinFHEContext>(absl::Span<const bool> value, const lbcrypto::BinFHEContext* cc, lbcrypto::LWECiphertext* out) {
  ::Unencrypted(value, *cc, out);
}

template<>
void Encrypt<lbcrypto::LWECiphertext, PalisadePrivateKeySet>(absl::Span<const bool> value, const PalisadePrivateKeySet* key, lbcrypto::LWECiphertext* out) {
  ::Encrypt(value, key, out);
}

template<>
void Decrypt<lbcrypto::LWECiphertext, PalisadePrivateKeySet>(lbcrypto::LWECiphertext* ciphertext, const PalisadePrivateKeySet* key, absl::Span<bool> plaintext) {
  ::Decrypt(ciphertext, key, plaintext);
}

$2
#endif//$0)";

// PALISADE struct template
//  0: Header guard
//  1: Type name
constexpr const char kPalisadeStructTemplate[] = R"(
using PalisadeBase$0 = GenericEncoded$0<lbcrypto::LWECiphertext, std::default_delete<lbcrypto::LWECiphertext[]>, PalisadePrivateKeySet, lbcrypto::BinFHEContext>;
class Palisade$0 : public PalisadeBase$0 {
 public:
  Palisade$0(lbcrypto::BinFHEContext cc)
      : PalisadeBase$0(
            new lbcrypto::LWECiphertext[Palisade$0::bit_width()],
            std::default_delete<lbcrypto::LWECiphertext[]>()), cc_(cc) {
    // Initialize the LWECiphertexts.
    SetUnencrypted({}, &cc_);
  }

  void SetEncrypted(const $1& value,
                    lbcrypto::LWEPrivateKey sk) {
    PalisadePrivateKeySet key{cc_, sk};
    PalisadeBase$0::SetEncrypted(value, &key);
  }

  $1 Decrypt(lbcrypto::LWEPrivateKey sk) {
    PalisadePrivateKeySet key{cc_, sk};
    return PalisadeBase$0::Decrypt(&key);
  }

 private:
  lbcrypto::BinFHEContext cc_;
};)";

absl::StatusOr<std::string> ConvertStructsToEncodedPalisade(
    absl::string_view generic_header,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view output_path) {
  if (metadata.structs_size() == 0) {
    return "";
  }
  std::string header_guard = GenerateHeaderGuard(output_path);
  std::vector<int64_t> struct_order = GetTypeReferenceOrder(metadata);
  IdToType id_to_type = PopulateTypeData(metadata, struct_order);
  std::vector<std::string> generated;
  for (int64_t id : struct_order) {
    const StructType& struct_type = id_to_type.at(id).type;
    xlscc_metadata::CPPName struct_name = struct_type.name().as_inst().name();
    std::string struct_text =
        absl::Substitute(kPalisadeStructTemplate, struct_name.name(),
                         struct_name.fully_qualified_name());
    generated.push_back(struct_text);
  }

  return absl::Substitute(kPalisadeFileTemplate, header_guard, generic_header,
                          absl::StrJoin(generated, "\n\n"));
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
