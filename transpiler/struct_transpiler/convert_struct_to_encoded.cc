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
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
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
  lines.push_back(absl::Substitute("        for (int $0 = 0; $0 < $1; $0++) {",
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
  lines.push_back("        }");

  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateSetOrEncryptBoolElement(
    absl::string_view source, bool encrypt) {
  std::vector<std::string> lines;
  std::string op = encrypt ? "EncryptFn" : "UnencryptedFn";
  lines.push_back(
      absl::Substitute("        $0(EncodedValue<bool>(value.$1).get(), key, "
                       "absl::MakeSpan(data, 1));",
                       op, source));
  lines.push_back("        data += 1;");
  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateSetOrEncryptIntegralElement(
    const Type& type, absl::string_view source_var, bool encrypt) {
  std::vector<std::string> lines;
  std::string op = encrypt ? "EncryptFn" : "UnencryptedFn";
  IntType int_type = type.as_int();
  XLS_ASSIGN_OR_RETURN(std::string int_type_name,
                       XlsccToNativeIntegerType(int_type));

  lines.push_back(
      absl::Substitute("        $0(EncodedValue<$1>(value.$2).get(), key, "
                       "absl::MakeSpan(data, $3));",
                       op, int_type_name, source_var, int_type.width()));
  lines.push_back(absl::Substitute("        data += $0;", int_type.width()));

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
  lines.push_back(absl::Substitute(
      "        $2<Sample, SampleArrayDeleter, SecretKey, "
      "PublicKey, BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, "
      "DecryptFn>::Borrowed$0(value.$1, data, key);",
      op, source_var, struct_name));
  lines.push_back(absl::Substitute("        data += $0;", type_data.bit_width));

  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateSetOrEncryptFunction(
    const IdToType& id_to_type, const StructType& struct_type, bool encrypt) {
  std::vector<std::string> lines;
  std::vector<std::string> reversed_buffer_lines;
  for (int i = 0; i < struct_type.fields_size(); i++) {
    XLS_ASSIGN_OR_RETURN(std::string line,
                         GenerateSetOrEncryptOneElement(
                             id_to_type, struct_type.fields(i), encrypt));
    lines.push_back(line);
    XLS_ASSIGN_OR_RETURN(std::string reverse_buffer_line,
                         GenerateSetOrEncryptOneElement(
                             id_to_type, struct_type.fields(i), encrypt));
    reversed_buffer_lines.push_back(reverse_buffer_line);
  }
  std::reverse(reversed_buffer_lines.begin(), reversed_buffer_lines.end());
  return absl::StrCat(
      "    if (GetStructEncodeOrder() == StructEncodeOrder::REVERSE) {\n",
      absl::StrJoin(lines, "\n"), "\n    } else {\n",
      absl::StrJoin(reversed_buffer_lines, "\n"), "\n    }");
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
  lines.push_back(absl::Substitute("        for (int $0 = 0; $0 < $1; $0++) {",
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
  lines.push_back("        }");

  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateDecryptBool(const IdToType& id_to_type,
                                                const std::string& temp_name,
                                                absl::string_view output_loc) {
  std::vector<std::string> lines;
  lines.push_back(
      absl::StrCat("        EncodedValue<bool> encoded_", temp_name, ";"));
  lines.push_back(absl::Substitute(
      "        DecryptFn(absl::MakeConstSpan(data, 1), key, encoded_$0.get());",
      temp_name));
  lines.push_back("        data += 1;");
  lines.push_back(absl::Substitute("        result->$0 = encoded_$1.Decode();",
                                   output_loc, temp_name));
  return absl::StrJoin(lines, "\n");
}
absl::StatusOr<std::string> GenerateDecryptIntegral(
    const IdToType& id_to_type, const IntType& int_type,
    const std::string& temp_name, absl::string_view output_loc) {
  std::vector<std::string> lines;
  XLS_ASSIGN_OR_RETURN(std::string int_type_name,
                       XlsccToNativeIntegerType(int_type));
  lines.push_back(absl::Substitute("        EncodedValue<$0> encoded_$1;",
                                   int_type_name, temp_name));
  lines.push_back(
      absl::Substitute("        DecryptFn(absl::MakeConstSpan(data, $1), key, "
                       "encoded_$0.get());",
                       temp_name, int_type.width()));
  lines.push_back(absl::Substitute("        data += $0;", int_type.width()));
  lines.push_back(absl::Substitute("        result->$0 = encoded_$1.Decode();",
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
  lines.push_back(absl::Substitute(
      "        $0<Sample, SampleArrayDeleter, SecretKey, "
      "PublicKey, BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, "
      "DecryptFn>::BorrowedDecrypt(data, &result->$1, key);",
      struct_name, output_loc));
  lines.push_back(absl::Substitute("        data += $0;", type_data.bit_width));
  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateDecryptFunction(
    const IdToType& id_to_type, const StructType& struct_type) {
  std::vector<std::string> lines;
  std::vector<std::string> reversed_buffer_lines;
  for (int i = 0; i < struct_type.fields_size(); i++) {
    const StructField& field = struct_type.fields(i);
    XLS_ASSIGN_OR_RETURN(std::string line,
                         GenerateDecryptOneElement(
                             id_to_type, field, struct_type.fields(i).name()));
    XLS_ASSIGN_OR_RETURN(std::string reversed_buffer_line,
                         GenerateDecryptOneElement(
                             id_to_type, field, struct_type.fields(i).name()));
    lines.push_back(line);
    reversed_buffer_lines.push_back(reversed_buffer_line);
  }
  std::reverse(reversed_buffer_lines.begin(), reversed_buffer_lines.end());
  return absl::StrCat(
      "    if (GetStructEncodeOrder() == StructEncodeOrder::REVERSE) {\n",
      absl::StrJoin(lines, "\n"), "\n    } else {\n",
      absl::StrJoin(reversed_buffer_lines, "\n"), "\n    }");
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

#include "xls/common/logging/logging.h"
#include "absl/types/span.h"
#include "transpiler/common_runner.h"
#include "transpiler/data/cleartext_value.h"
$0

template <class Sample, class BootstrappingKey>
using CopyFnT = void(absl::Span<const Sample> value, const BootstrappingKey* key, absl::Span<Sample> out);

template <class Sample, class PublicKey>
using UnencryptedFnT = void(absl::Span<const bool> value, const PublicKey* key, absl::Span<Sample> out);

template <class Sample, class SecretKey>
using EncryptFnT = void(absl::Span<const bool> value, const SecretKey* key, absl::Span<Sample> out);

template <class Sample, class SecretKey>
using DecryptFnT = void(absl::Span<const Sample> ciphertext, const SecretKey* key, absl::Span<bool> plaintext);

$1
#endif//$2)";

// The template for each individual struct. Substitution params:
//  0: The name of the struct we're transpiling.
//  1: The fully-qualified name of the struct we're transpiling.
//  2: The body of the internal "Set" routine.
//  3: The body of the internal "Encrypt" routine.
//  4: The body of the internal "Decrypt" routine.
//  5: The [packed] bit width of the structure.
constexpr const char kClassTemplate[] = R"(
template <class Sample, class BootstrappingKey>
using Copy$0FnT = void(absl::Span<const Sample> value, const BootstrappingKey* key, absl::Span<Sample> out);

template <class Sample, class PublicKey>
using Unencrypted$0FnT = void(absl::Span<const bool> value, const PublicKey* key, absl::Span<Sample> out);

template <class Sample, class SecretKey>
using Encrypt$0FnT = void(absl::Span<const bool> value, const SecretKey* key, absl::Span<Sample> out);

template <class Sample, class SecretKey>
using Decrypt$0FnT = void(absl::Span<const Sample> ciphertext, const SecretKey* key, absl::Span<bool> plaintext);

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          Copy$0FnT<Sample, BootstrappingKey> CopyFn,
          Unencrypted$0FnT<Sample, PublicKey> UnencryptedFn,
          Encrypt$0FnT<Sample, SecretKey> EncryptFn,
          Decrypt$0FnT<Sample, SecretKey> DecryptFn>
class GenericEncoded$0Ref {
 public:
  GenericEncoded$0Ref(Sample* data, size_t length)
      : length_(length), data_(data) {}

  // We set values here directly, instead of using EncodedValue, since
  // EncodedValue types own their arrays, whereas we'd need to own them here as
  // contiguously-allocated chunks. (We could modify them to use borrowed data,
  // but it'd be more work than this). For structure types, though, we do
  // enable setting on "borrowed" data to enable recursive setting.
  void SetUnencrypted(const $1& value, const PublicKey* key, size_t elem = 0) {
    SetUnencryptedInternal(value, key, data_ + elem * element_bit_width());
  }

  void SetEncrypted(const $1& value, const SecretKey* key, size_t elem = 0) {
    SetEncryptedInternal(value, key, data_ + elem * element_bit_width());
  }

  $1 Decrypt(const SecretKey* key, size_t elem = 0) const {
    $1 result;
    DecryptInternal(key, data_ + elem * element_bit_width(), &result);
    return result;
  }

  static void BorrowedSetUnencrypted(const $1& value, const PublicKey* key,
                                     Sample* data) {
    SetUnencryptedInternal(value, key, data);
  }

  static void BorrowedSetEncrypted(const $1& value, const SecretKey* key,
                                   Sample* data) {
    SetEncryptedInternal(value, key, data);
  }

  static void BorrowedDecrypt(const SecretKey* key, Sample* data, $1* result) {
    DecryptInternal(key, data, result);
  }

  absl::Span<Sample> get() { return absl::MakeSpan(data_, bit_width()); }
  absl::Span<const Sample> get() const {
    return absl::MakeConstSpan(data_, bit_width());
  }

  size_t length() const { return length_; }
  size_t bit_width() const { return length_ * element_bit_width(); }
  static constexpr size_t element_bit_width() { return element_bit_width_; }

 private:
  static constexpr const size_t element_bit_width_ = $5;
  static void SetUnencryptedInternal(const $1& value, const PublicKey* key,
                                     Sample* data) {
$2
  }

  static void SetEncryptedInternal(const $1& value, const SecretKey* key,
                                   Sample* data) {
$3
  }

  static void DecryptInternal(const SecretKey* key, Sample* data,
                              $1* result){$4}

  size_t length_;
  Sample* data_;
};

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          Copy$0FnT<Sample, BootstrappingKey> CopyFn,
          Unencrypted$0FnT<Sample, PublicKey> UnencryptedFn,
          Encrypt$0FnT<Sample, SecretKey> EncryptFn,
          Decrypt$0FnT<Sample, SecretKey> DecryptFn>
class GenericEncoded$0 {
 public:
  GenericEncoded$0(const $1& value, const PublicKey* key) {
    SetUnencrypted(value, key);
  }

  GenericEncoded$0(Sample* data, size_t length, SampleArrayDeleter deleter)
      : length_(length), data_(data, deleter) {}
  GenericEncoded$0(GenericEncoded$0&&) = default;

  operator const GenericEncoded$0Ref<
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn>() const& {
    return GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>(data_.get(),
                                                     this->length());
  }
  operator GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>() & {
    return GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>(data_.get(),
                                                     this->length());
  }

  // We set values here directly, instead of using EncodedValue, since
  // EncodedValue types own their arrays, whereas we'd need to own them here as
  // contiguously-allocated chunks. (We could modify them to use borrowed data,
  // but it'd be more work than this). For structure types, though, we do
  // enable setting on "borrowed" data to enable recursive setting.
  void SetUnencrypted(const $1& value, const PublicKey* key, size_t elem = 0) {
    SetUnencryptedInternal(value, key, data_.get() + elem * element_bit_width());
  }

  void SetEncrypted(const $1& value, const SecretKey* key, size_t elem = 0) {
    XLS_CHECK(elem < this->length());
    SetEncryptedInternal(value, key, data_.get() + elem * element_bit_width());
  }

  $1 Decrypt(const SecretKey* key, size_t elem = 0) const {
    XLS_CHECK(elem < this->length());
    $1 result;
    DecryptInternal(key, data_.get() + elem * element_bit_width(), &result);
    return result;
  }

  static void BorrowedSetUnencrypted(const $1& value, Sample* data,
                                     const PublicKey* key) {
    SetUnencryptedInternal(value, key, data);
  }

  static void BorrowedSetEncrypted(const $1& value, Sample* data,
                                   const SecretKey* key) {
    SetEncryptedInternal(value, key, data);
  }

  static void BorrowedDecrypt(Sample* data, $1* result, const SecretKey* key) {
    DecryptInternal(key, data, result);
  }

  absl::Span<Sample> get() { return absl::MakeSpan(data_.get(), bit_width()); }
  absl::Span<const Sample> get() const {
    return absl::MakeConstSpan(data_.get(), bit_width());
  }

  size_t length() const { return length_; }
  size_t bit_width() const { return length_ * element_bit_width(); }
  static constexpr size_t element_bit_width() { return element_bit_width_; }

 private:
  static constexpr const size_t element_bit_width_ = $5;

  static void SetUnencryptedInternal(const $1& value, const PublicKey* key,
                                     Sample* data) {
$2
  }

  static void SetEncryptedInternal(const $1& value, const SecretKey* key,
                                   Sample* data) {
$3
  }

  static void DecryptInternal(const SecretKey* key, Sample* data,
                              $1* result){$4}

  size_t length_;
  std::unique_ptr<Sample[], SampleArrayDeleter> data_;
};

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          Copy$0FnT<Sample, BootstrappingKey> CopyFn,
          Unencrypted$0FnT<Sample, PublicKey> UnencryptedFn,
          Encrypt$0FnT<Sample, SecretKey> EncryptFn,
          Decrypt$0FnT<Sample, SecretKey> DecryptFn, unsigned... Dimensions>
class GenericEncoded$0Array;

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          Copy$0FnT<Sample, BootstrappingKey> CopyFn,
          Unencrypted$0FnT<Sample, PublicKey> UnencryptedFn,
          Encrypt$0FnT<Sample, SecretKey> EncryptFn,
          Decrypt$0FnT<Sample, SecretKey> DecryptFn, unsigned... Dimensions>
class GenericEncoded$0ArrayRef;

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          Copy$0FnT<Sample, BootstrappingKey> CopyFn,
          Unencrypted$0FnT<Sample, PublicKey> UnencryptedFn,
          Encrypt$0FnT<Sample, SecretKey> EncryptFn,
          Decrypt$0FnT<Sample, SecretKey> DecryptFn>
class GenericEncoded$0Array<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn>
    : public GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                              BootstrappingKey, CopyFn, UnencryptedFn,
                              EncryptFn, DecryptFn> {
 public:
  using GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                         BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                         DecryptFn>::GenericEncoded$0;

  operator const GenericEncoded$0ArrayRef<
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn>() const& {
    return GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn>(
        this->get(), this->length());
  }
  operator GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn>() & {
    return GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn>(
        this->get(), this->length());
  }

  template <unsigned D1>
  operator const GenericEncoded$0ArrayRef<
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1>() const& {
    return GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn, D1>(
        this->get(), this->length());
  }
  template <unsigned D1>
  operator GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn, D1>() & {
    return GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn, D1>(
        this->get(), this->length());
  }

  // We set values here directly, instead of using EncodedValue, since
  // EncodedValue types own their arrays, whereas we'd need to own them here as
  // contiguously-allocated chunks. (We could modify them to use borrowed data,
  // but it'd be more work than this). For structure types, though, we do
  // enable setting on "borrowed" data to enable recursive setting.
  void SetUnencrypted(const $1* value, size_t len, const PublicKey* key) {
    XLS_CHECK(this->length() >= len);
    for (size_t i = 0; i < len; i++) {
      GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                       BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                       DecryptFn>::SetUnencrypted(value[i], key, i);
    }
  }

  void SetEncrypted(const $1* value, size_t len, const SecretKey* key) {
    XLS_CHECK(this->length() >= len);
    for (size_t i = 0; i < len; i++) {
      GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                       BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                       DecryptFn>::SetEncrypted(value[i], key, i);
    }
  }

  void SetEncrypted(absl::Span<const $1> values, const SecretKey* key) {
    XLS_CHECK(this->length() >= values.size());
    for (size_t i = 0; i < values.size(); i++) {
      GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                       BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                       DecryptFn>::SetEncrypted(values[i], key, i);
    }
  }

  void Decrypt($1* result, size_t len, const SecretKey* key) const {
    XLS_CHECK(len >= this->length());
    for (size_t i = 0; i < this->length(); i++) {
      result[i] =
          GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                           BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                           DecryptFn>::Decrypt(key, i);
    }
  }

  absl::FixedArray<$1> Decrypt(const SecretKey* key) const {
    absl::FixedArray<$1> plaintext(this->length());
    Decrypt(plaintext.data(), this->length(), key);
    return plaintext;
  }

  using GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                         BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn>::get;

  GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                      BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                      DecryptFn>
  operator[](size_t pos) {
    XLS_CHECK(pos < this->length());
    auto span = this->get();
    return GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>(span.data() + pos * $5, 1);
  }
};

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          Copy$0FnT<Sample, BootstrappingKey> CopyFn,
          Unencrypted$0FnT<Sample, PublicKey> UnencryptedFn,
          Encrypt$0FnT<Sample, SecretKey> EncryptFn,
          Decrypt$0FnT<Sample, SecretKey> DecryptFn, unsigned D1>
class GenericEncoded$0Array<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, D1>
    : public GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                              BootstrappingKey, CopyFn, UnencryptedFn,
                              EncryptFn, DecryptFn> {
 public:
  using GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                         BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                         DecryptFn>::GenericEncoded$0;
  enum { VOLUME = D1 };
  using ArrayT = $1[D1];

  operator const GenericEncoded$0ArrayRef<
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1>() const& {
    return GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn, D1>(
        this->get(), this->length());
  }
  operator GenericEncoded$0ArrayRef<
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1>() & {
    return GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn, D1>(
        this->get(), this->length());
  }

  void SetUnencrypted(const ArrayT value, const PublicKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                       BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                       DecryptFn>::SetUnencrypted(value[i], key, elem * D1 + i);
    }
  }

  void SetEncrypted(const ArrayT value, const SecretKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                       BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                       DecryptFn>::SetEncrypted(value[i], key, elem * D1 + i);
    }
  }

  void Decrypt(ArrayT result, const SecretKey* key, size_t elem = 0) const {
    for (size_t i = 0; i < D1; i++) {
      result[i] =
          GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                           BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                           DecryptFn>::Decrypt(key, elem * D1 + i);
    }
  }

  absl::FixedArray<$1> Decrypt(const SecretKey* key) const {
    absl::FixedArray<$1> plaintext(this->length());
    Decrypt(plaintext.data(), key);
    return plaintext;
  }

  using GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                         BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn>::get;

  GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                      BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                      DecryptFn>
  operator[](size_t pos) {
    XLS_CHECK(pos < this->length());
    auto span = this->get();
    return GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>(span.data() + pos * $5, 1);
  }
  // The base will return the full length of the underlying array, when we want
  // to return the length of this level of the multi-dimensional array.
  size_t length() const { return D1; }
};

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          Copy$0FnT<Sample, BootstrappingKey> CopyFn,
          Unencrypted$0FnT<Sample, PublicKey> UnencryptedFn,
          Encrypt$0FnT<Sample, SecretKey> EncryptFn,
          Decrypt$0FnT<Sample, SecretKey> DecryptFn, unsigned D1,
          unsigned... Dimensions>
class GenericEncoded$0Array<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, D1, Dimensions...>
    : public GenericEncoded$0Array<
          Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
          CopyFn, UnencryptedFn, EncryptFn, DecryptFn, Dimensions...> {
 public:
  using GenericEncoded$0Array<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                              BootstrappingKey, CopyFn, UnencryptedFn,
                              EncryptFn, DecryptFn,
                              Dimensions...>::GenericEncoded$0Array;
  using LowerT =
      GenericEncoded$0Array<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, Dimensions...>;
  using LowerArrayT = typename LowerT::ArrayT;
  enum { VOLUME = D1 * LowerT::VOLUME };
  using ArrayT = LowerArrayT[D1];

  operator const GenericEncoded$0ArrayRef<
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1, Dimensions...>() const& {
    return GenericEncoded$0ArrayRef<
        Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
        CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1, Dimensions...>(
        this->get(), this->length());
  }
  operator GenericEncoded$0ArrayRef<
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1, Dimensions...>() & {
    return GenericEncoded$0ArrayRef<
        Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
        CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1, Dimensions...>(
        this->get(), this->length());
  }

  void SetUnencrypted(const ArrayT value, const PublicKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded$0Array<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn,
                            Dimensions...>::SetUnencrypted(value[i], key,
                                                           elem * D1 + i);
    }
  }

  void SetEncrypted(const ArrayT value, const SecretKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded$0Array<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, Dimensions...>::SetEncrypted(value[i],
                                                                    key,
                                                                    elem * D1 +
                                                                        i);
    }
  }

  void Decrypt(ArrayT result, const SecretKey* key, size_t elem = 0) const {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded$0Array<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, Dimensions...>::Decrypt(result[i], key,
                                                               elem * D1 + i);
    }
  }

  using GenericEncoded$0<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                         BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn>::get;

  GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                           BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                           DecryptFn, Dimensions...>
  operator[](size_t pos) {
    XLS_CHECK(pos < this->length());
    auto span = this->get();
    return GenericEncoded$0ArrayRef<
        Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
        CopyFn, UnencryptedFn, EncryptFn, DecryptFn, Dimensions...>(
        span.data() + pos * LowerT::VOLUME * $5, D1);
  }
  // The base will return the full length of the underlying array, when we want
  // to return the length of this level of the multi-dimensional array.
  size_t length() const { return D1; }
};

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          Copy$0FnT<Sample, BootstrappingKey> CopyFn,
          Unencrypted$0FnT<Sample, PublicKey> UnencryptedFn,
          Encrypt$0FnT<Sample, SecretKey> EncryptFn,
          Decrypt$0FnT<Sample, SecretKey> DecryptFn>
class GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>
    : public GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey,
                                 PublicKey, BootstrappingKey, CopyFn,
                                 UnencryptedFn, EncryptFn, DecryptFn> {
 public:
  GenericEncoded$0ArrayRef(Sample* data, size_t length)
      : GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn>(data, length) {}

  void SetUnencrypted(const $1* value, size_t len, const SecretKey* key) {
    XLS_CHECK(this->length() >= len);
    for (size_t i = 0; i < len; i++) {
      GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                          BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                          DecryptFn>::SetUnencrypted(value[i], key, i);
    }
  }

  void SetEncrypted(const $1* value, size_t len, const SecretKey* key) {
    XLS_CHECK(this->length() >= len);
    for (size_t i = 0; i < len; i++) {
      GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                          BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                          DecryptFn>::SetEncrypted(value[i], key, i);
    }
  }

  void Decrypt($1* result, size_t len, const SecretKey* key) const {
    XLS_CHECK(len >= this->length());
    for (size_t i = 0; i < this->length(); i++) {
      result[i] =
          GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                              BootstrappingKey, CopyFn, UnencryptedFn,
                              EncryptFn, DecryptFn>::Decrypt(key, i);
    }
  }

  absl::FixedArray<$1> Decrypt(const SecretKey* key) const {
    absl::FixedArray<$1> plaintext(this->length());
    Decrypt(plaintext.data(), this->length(), key);
    return plaintext;
  }

  using GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey,
                            PublicKey, BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn>::get;

  GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                      BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                      DecryptFn>
  operator[](size_t pos) {
    XLS_CHECK(pos < this->length());
    auto span = this->get();
    return GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>(span.data() + pos * $5, 1);
  }
};

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          Copy$0FnT<Sample, BootstrappingKey> CopyFn,
          Unencrypted$0FnT<Sample, PublicKey> UnencryptedFn,
          Encrypt$0FnT<Sample, SecretKey> EncryptFn,
          Decrypt$0FnT<Sample, SecretKey> DecryptFn, unsigned D1>
class GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn, D1>
    : public GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey,
                                 PublicKey, BootstrappingKey, CopyFn,
                                 UnencryptedFn, EncryptFn, DecryptFn> {
 public:
  enum { VOLUME = D1 };
  using ArrayT = $1[D1];

  GenericEncoded$0ArrayRef(Sample* data, size_t length)
      : GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn>(data, length) {}

  void SetUnencrypted(const ArrayT value, const SecretKey* key, size_t elem) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                          BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                          DecryptFn>::SetUnencrypted(value[i], key,
                                                     elem * D1 + i);
    }
  }

  void SetEncrypted(const ArrayT value, const SecretKey* key, size_t elem) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                          BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                          DecryptFn>::SetEncrypted(value[i], key,
                                                   elem * D1 + i);
    }
  }

  void Decrypt(ArrayT result, const SecretKey* key, size_t elem = 0) const {
    for (size_t i = 0; i < D1; i++) {
      result[i] =
          GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                              BootstrappingKey, CopyFn, UnencryptedFn,
                              EncryptFn, DecryptFn>::Decrypt(key,
                                                             elem * D1 + i);
    }
  }

  absl::FixedArray<$1> Decrypt(const SecretKey* key) const {
    absl::FixedArray<$1> plaintext(this->length());
    Decrypt(plaintext.data(), this->length(), key);
    return plaintext;
  }

  using GenericEncoded$0Ref<Sample, SampleArrayDeleter, SecretKey,
                            PublicKey, BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn>::get;

  // The base will return the full length of the underlying array, when we want
  // to return the length of this level of the multi-dimensional array.
  size_t length() const { return D1; }
};

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          Copy$0FnT<Sample, BootstrappingKey> CopyFn,
          Unencrypted$0FnT<Sample, PublicKey> UnencryptedFn,
          Encrypt$0FnT<Sample, SecretKey> EncryptFn,
          Decrypt$0FnT<Sample, SecretKey> DecryptFn, unsigned D1,
          unsigned... Dimensions>
class GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn, D1, Dimensions...>
    : public GenericEncoded$0ArrayRef<
          Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
          CopyFn, UnencryptedFn, EncryptFn, DecryptFn, Dimensions...> {
 public:
  using LowerT =
      GenericEncoded$0Array<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, Dimensions...>;
  using LowerArrayT = typename LowerT::ArrayT;
  enum { VOLUME = D1 * LowerT::VOLUME };
  using ArrayT = LowerArrayT[D1];

  GenericEncoded$0ArrayRef(Sample* data, size_t length)
      : GenericEncoded$0ArrayRef<
            Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
            CopyFn, UnencryptedFn, EncryptFn, DecryptFn, Dimensions...>(
            data, length) {}

  void SetUnencrypted(const ArrayT value, const SecretKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn,
                               Dimensions...>::SetUnencrypted(value[i], key,
                                                              elem * D1 + i);
    }
  }

  void SetEncrypted(const ArrayT value, const SecretKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn,
                               Dimensions...>::SetEncrypted(value[i], key,
                                                            elem * D1 + i);
    }
  }

  void Decrypt(ArrayT result, const SecretKey* key, size_t elem = 0) const {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn,
                               Dimensions...>::Decrypt(result[i], key,
                                                       elem * D1 + i);
    }
  }

  using GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey,
                            PublicKey, BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn,
                            Dimensions...>::get;

  GenericEncoded$0ArrayRef<Sample, SampleArrayDeleter, SecretKey, PublicKey,
                           BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                           DecryptFn, Dimensions...>
  operator[](size_t pos) {
    XLS_CHECK(pos < this->length());
    auto span = this->get();
    return GenericEncoded$0ArrayRef<
        Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
        CopyFn, UnencryptedFn, EncryptFn, DecryptFn, Dimensions...>(
        span.data() + pos * LowerT::VOLUME * $5, D1);
  }
  // The base will return the full length of the underlying array, when we want
  // to return the length of this level of the multi-dimensional array.
  size_t length() const { return D1; }
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

absl::StatusOr<std::string> ConvertStructsToEncodedTemplate(
    const xlscc_metadata::MetadataOutput& metadata,
    const std::vector<std::string>& original_headers,
    absl::string_view output_path) {
  if (metadata.structs_size() == 0) {
    return "";
  }

  std::string header_guard = GenerateHeaderGuard(output_path);

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
                          absl::StrJoin(generated, "\n\n"), header_guard);
}

// Bool header template
//  0: Header guard
//  1: Generic header
//  2: Class definitions
constexpr const char kBoolFileTemplate[] = R"(#ifndef $0
#define $0

#include <memory>

#include "xls/common/logging/logging.h"
#include "transpiler/data/cleartext_value.h"
#include "$1"
#include "absl/types/span.h"

$2
#endif//$0)";

// Bool struct template
//  0: Struct name
//  1: Fully-qualified struct-type name
constexpr const char kBoolStructTemplate[] = R"(
using EncodedBase$0Ref =
    GenericEncoded$0Ref<bool, std::default_delete<bool[]>, void, void, void,
                        ::CleartextCopy, ::CleartextEncode, ::CleartextEncode,
                        ::CleartextDecode>;
class Encoded$0Ref : public EncodedBase$0Ref {
 public:
  using EncodedBase$0Ref::EncodedBase$0Ref;

  Encoded$0Ref(const EncodedBase$0Ref& rhs)
      : Encoded$0Ref(const_cast<bool*>(rhs.get().data()), rhs.length()) {}

  void Encode(const $1& value) { SetEncrypted(value, nullptr); }

  $1 Decode() const { return Decrypt(nullptr); }

  using EncodedBase$0Ref::get;

 private:
  using EncodedBase$0Ref::BorrowedDecrypt;
  using EncodedBase$0Ref::BorrowedSetEncrypted;
  using EncodedBase$0Ref::BorrowedSetUnencrypted;
  using EncodedBase$0Ref::Decrypt;
  using EncodedBase$0Ref::SetEncrypted;
  using EncodedBase$0Ref::SetUnencrypted;
};

using EncodedBase$0 =
    GenericEncoded$0<bool, std::default_delete<bool[]>, void, void, void,
                     ::CleartextCopy, ::CleartextEncode, ::CleartextEncode,
                     ::CleartextDecode>;
class Encoded$0 : public EncodedBase$0 {
 public:
  Encoded$0()
      : EncodedBase$0(new bool[Encoded$0::element_bit_width()], 1,
                      std::default_delete<bool[]>()) {}
  Encoded$0(const $1& value) : Encoded$0() { Encode(value); }

  Encoded$0& operator=(const Encoded$0Ref rhs) {
    ::CleartextCopy(rhs.get(), nullptr, this->get());
    return *this;
  }

  operator const Encoded$0Ref() const& {
    return Encoded$0Ref(const_cast<bool*>(get().data()), this->length());
  }
  operator Encoded$0Ref() & {
    return Encoded$0Ref(const_cast<bool*>(get().data()), this->length());
  }

  void Encode(const $1& value) { SetEncrypted(value, nullptr); }

  $1 Decode() const { return Decrypt(nullptr); }

  using EncodedBase$0::get;

 private:
  using EncodedBase$0::BorrowedDecrypt;
  using EncodedBase$0::BorrowedSetEncrypted;
  using EncodedBase$0::BorrowedSetUnencrypted;
  using EncodedBase$0::Decrypt;
  using EncodedBase$0::SetEncrypted;
  using EncodedBase$0::SetUnencrypted;
};

template <unsigned... Dimensions>
using EncodedBase$0Array =
    GenericEncoded$0Array<bool, std::default_delete<bool[]>, void, void, void,
                          ::CleartextCopy, ::CleartextEncode, ::CleartextEncode,
                          ::CleartextDecode, Dimensions...>;

template <unsigned... Dimensions>
class Encoded$0Array;

template <>
class Encoded$0Array<> : public EncodedBase$0Array<> {
 public:
  Encoded$0Array(size_t length)
      : EncodedBase$0Array<>(new bool[length * Encoded$0::element_bit_width()],
                             length, std::default_delete<bool[]>()) {}

  Encoded$0Array(std::initializer_list<$1> values)
      : Encoded$0Array(values.size()) {
    Encode(std::data(values), values.size());
  }

  void Encode(const $1* value, size_t length) {
    XLS_CHECK(this->length() >= length);
    SetEncrypted(value, length, nullptr);
  }

  void Encode(absl::Span<const $1> values) {
    XLS_CHECK(this->length() >= values.size());
    SetEncrypted(values.data(), values.size(), nullptr);
  }

  void Decode($1* value, size_t length) const {
    XLS_CHECK(length >= this->length());
    Decrypt(value, length, nullptr);
  }

  absl::FixedArray<$1> Decode() const { return Decrypt(nullptr); }

  using EncodedBase$0Array<>::get;

 private:
  using EncodedBase$0Array<>::Decrypt;
  using EncodedBase$0Array<>::SetUnencrypted;
  using EncodedBase$0Array<>::SetEncrypted;
  using EncodedBase$0Array<>::BorrowedSetUnencrypted;
  using EncodedBase$0Array<>::BorrowedSetEncrypted;
  using EncodedBase$0Array<>::BorrowedDecrypt;
};

template <unsigned D1>
class Encoded$0Array<D1> : public EncodedBase$0Array<D1> {
 public:
  Encoded$0Array()
      : EncodedBase$0Array<D1>(
            new bool[EncodedBase$0Array<D1>::element_bit_width() * D1], D1,
            std::default_delete<bool[]>()) {}

  Encoded$0Array(std::initializer_list<$1> values)
      : Encoded$0Array<D1>() {
    XLS_CHECK_EQ(values.size(), D1);
    Encode(std::data(values));
  }

  Encoded$0Array(const $1 values[D1])
      : Encoded$0Array<D1>() {
    Encode(values);
  }

  void Encode(std::add_const_t<typename EncodedBase$0Array<D1>::ArrayT> value) {
    SetEncrypted(value, nullptr, 0);
  }

  void Decode(typename EncodedBase$0Array<D1>::ArrayT value) const {
    Decrypt(value, nullptr);
  }

  absl::FixedArray<$1> Decode() const { return Decrypt(nullptr); }

  using EncodedBase$0Array<D1>::operator[];

  using EncodedBase$0Array<D1>::get;

 private:
  using EncodedBase$0Array<D1>::Decrypt;
  using EncodedBase$0Array<D1>::SetUnencrypted;
  using EncodedBase$0Array<D1>::SetEncrypted;
  using EncodedBase$0Array<D1>::BorrowedSetUnencrypted;
  using EncodedBase$0Array<D1>::BorrowedSetEncrypted;
  using EncodedBase$0Array<D1>::BorrowedDecrypt;
};

template <unsigned... Dimensions>
class Encoded$0Array : public EncodedBase$0Array<Dimensions...> {
 public:
  Encoded$0Array()
      : EncodedBase$0Array<Dimensions...>(
            new bool[EncodedBase$0Array<Dimensions...>::element_bit_width() *
                     EncodedBase$0Array<Dimensions...>::VOLUME],
            EncodedBase$0Array<Dimensions...>::VOLUME,
            std::default_delete<bool[]>()) {}

  // No initializer_list constructor for multi-dimensional arrays.

  void Encode(std::add_const_t<typename EncodedBase$0Array<Dimensions...>::ArrayT> value) {
    SetEncrypted(value, nullptr, 0);
  }

  void Decode(typename EncodedBase$0Array<Dimensions...>::ArrayT value) const {
    Decrypt(value, nullptr);
  }

  using EncodedBase$0Array<Dimensions...>::operator[];

  using EncodedBase$0Array<Dimensions...>::get;

 private:
  using EncodedBase$0Array<Dimensions...>::Decrypt;
  using EncodedBase$0Array<Dimensions...>::SetUnencrypted;
  using EncodedBase$0Array<Dimensions...>::SetEncrypted;
  using EncodedBase$0Array<Dimensions...>::BorrowedSetUnencrypted;
  using EncodedBase$0Array<Dimensions...>::BorrowedSetEncrypted;
  using EncodedBase$0Array<Dimensions...>::BorrowedDecrypt;
};

template <unsigned... Dimensions>
using EncodedBase$0ArrayRef = GenericEncoded$0ArrayRef<
    bool, std::default_delete<bool[]>, void, void, void, ::CleartextCopy,
    ::CleartextEncode, ::CleartextEncode, ::CleartextDecode, Dimensions...>;

template <unsigned... Dimensions>
class Encoded$0ArrayRef;

template <>
class Encoded$0ArrayRef<> : public EncodedBase$0ArrayRef<> {
 public:
  using EncodedBase$0ArrayRef<>::EncodedBase$0ArrayRef;
  Encoded$0ArrayRef(const EncodedBase$0ArrayRef<>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()), rhs.length()) {}
  Encoded$0ArrayRef(const EncodedBase$0Array<>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()), rhs.length()) {}

  void Encode(const $1* value, size_t length) {
    XLS_CHECK(this->length() >= length);
    SetEncrypted(value, length, nullptr);
  }

  void Decode($1* value, size_t length) const {
    XLS_CHECK(length >= this->length());
    Decrypt(value, length, nullptr);
  }

  absl::FixedArray<$1> Decode() const { return Decrypt(nullptr); }

  using EncodedBase$0ArrayRef<>::get;

 private:
  using EncodedBase$0ArrayRef<>::Decrypt;
  using EncodedBase$0ArrayRef<>::SetUnencrypted;
  using EncodedBase$0ArrayRef<>::SetEncrypted;
  using EncodedBase$0ArrayRef<>::BorrowedSetUnencrypted;
  using EncodedBase$0ArrayRef<>::BorrowedSetEncrypted;
  using EncodedBase$0ArrayRef<>::BorrowedDecrypt;
};

template <unsigned D1>
class Encoded$0ArrayRef<D1> : public EncodedBase$0ArrayRef<D1> {
 public:
  using EncodedBase$0ArrayRef<D1>::EncodedBase$0ArrayRef;

  Encoded$0ArrayRef(const EncodedBase$0ArrayRef<D1>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()), rhs.length()) {}
  Encoded$0ArrayRef(const EncodedBase$0Array<D1>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()), rhs.length()) {}

  void Encode(std::add_const_t<typename EncodedBase$0ArrayRef<D1>::ArrayT> value) {
    SetEncrypted(value, nullptr);
  }

  void Decode(typename EncodedBase$0ArrayRef<D1>::ArrayT value) const {
    Decrypt(value, nullptr);
  }

  absl::FixedArray<$1> Decode() const { return Decrypt(nullptr); }

  using EncodedBase$0ArrayRef<D1>::get;

 private:
  using EncodedBase$0ArrayRef<D1>::Decrypt;
  using EncodedBase$0ArrayRef<D1>::SetUnencrypted;
  using EncodedBase$0ArrayRef<D1>::SetEncrypted;
  using EncodedBase$0ArrayRef<D1>::BorrowedSetUnencrypted;
  using EncodedBase$0ArrayRef<D1>::BorrowedSetEncrypted;
  using EncodedBase$0ArrayRef<D1>::BorrowedDecrypt;
};

template <unsigned... Dimensions>
class Encoded$0ArrayRef : public EncodedBase$0ArrayRef<Dimensions...> {
 public:
  using EncodedBase$0ArrayRef<Dimensions...>::EncodedBase$0ArrayRef;
  // Use the VOLUME constant instead of length().  The former gives the total
  // volume of the underlying array (accounting for all dimensions), while the
  // former gives the length of the first level of the array (e.g.,
  // Type[4][3][2] will have VOLUME = 24 and length() == 4.
  Encoded$0ArrayRef(const EncodedBase$0ArrayRef<Dimensions...>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()),
                          EncodedBase$0ArrayRef<Dimensions...>::VOLUME) {}
  Encoded$0ArrayRef(const EncodedBase$0Array<Dimensions...>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()),
                          EncodedBase$0Array<Dimensions...>::VOLUME) {}

  void Encode(
      const typename EncodedBase$0ArrayRef<Dimensions...>::ArrayT value) {
    SetEncrypted(value, nullptr);
  }

  void Decode(
      typename EncodedBase$0ArrayRef<Dimensions...>::ArrayT value) const {
    Decrypt(value, nullptr);
  }

  using EncodedBase$0ArrayRef<Dimensions...>::get;

 private:
  using EncodedBase$0ArrayRef<Dimensions...>::Decrypt;
  using EncodedBase$0ArrayRef<Dimensions...>::SetUnencrypted;
  using EncodedBase$0ArrayRef<Dimensions...>::SetEncrypted;
  using EncodedBase$0ArrayRef<Dimensions...>::BorrowedSetUnencrypted;
  using EncodedBase$0ArrayRef<Dimensions...>::BorrowedSetEncrypted;
  using EncodedBase$0ArrayRef<Dimensions...>::BorrowedDecrypt;
};

template <>
class EncodedArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>>: public Encoded$0Array<> {
 public:
  using Encoded$0Array<>::Encoded$0Array;
  EncodedArray<$1>(std::initializer_list<$1> list) :
    Encoded$0Array(list) {}
};

template <unsigned D1>
class EncodedArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1> : public Encoded$0Array<D1> {
 public:
  using Encoded$0Array<D1>::Encoded$0Array;
  EncodedArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1>(std::initializer_list<$1> list) :
    Encoded$0Array<D1>(list) {}
};

template <unsigned... Dimensions>
class EncodedArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, Dimensions...> : public Encoded$0Array<Dimensions...> {};

template <>
class EncodedArrayRef<$1, typename std::enable_if_t<!std::is_integral_v<$1>>>: public Encoded$0ArrayRef<> {
public:
  using Encoded$0ArrayRef<>::Encoded$0ArrayRef;
  EncodedArrayRef(const EncodedArray<$1>& rhs)
      : EncodedArrayRef<$1>(const_cast<bool*>(rhs.get().data()), rhs.length()) {
  }
};

template <unsigned D1, unsigned... Dimensions>
class EncodedArrayRef<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1, Dimensions...>
    : public Encoded$0ArrayRef<D1, Dimensions...> {
 public:
  using Encoded$0ArrayRef<D1, Dimensions...>::Encoded$0ArrayRef;
  EncodedArrayRef(const EncodedArray<$1, std::enable_if_t<!std::is_integral_v<$1>>, D1, Dimensions...>& rhs)
      : EncodedArrayRef<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1, Dimensions...>(const_cast<bool*>(rhs.get().data()),
                        Encoded$0ArrayRef<D1, Dimensions...>::VOLUME) {

    XLS_CHECK_EQ(rhs.length(), D1);
  }
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
        absl::Substitute(kBoolStructTemplate, struct_name.name(),
                         struct_name.fully_qualified_name());
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

#include "transpiler/data/tfhe_value.h"
#include "$1"
#include "absl/types/span.h"
#include "tfhe/tfhe.h"

$2
#endif//$0)";

// Tfhe struct template
//  0: Type name
//  1: Fully-qualified type name
constexpr const char kTfheStructTemplate[] = R"(
using TfheBase$0Ref = GenericEncoded$0Ref<
    LweSample, LweSampleArrayDeleter, TFheGateBootstrappingSecretKeySet,
    TFheGateBootstrappingCloudKeySet, TFheGateBootstrappingParameterSet,
    ::TfheCopy, ::TfheUnencrypted, ::TfheEncrypt, ::TfheDecrypt>;
class Tfhe$0Ref : public TfheBase$0Ref {
 public:
  using TfheBase$0Ref::TfheBase$0Ref;

  Tfhe$0Ref(const TfheBase$0Ref& rhs)
      : Tfhe$0Ref(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {}
};

using TfheBase$0 = GenericEncoded$0<
    LweSample, LweSampleArrayDeleter, TFheGateBootstrappingSecretKeySet,
    TFheGateBootstrappingCloudKeySet, TFheGateBootstrappingParameterSet,
    ::TfheCopy, ::TfheUnencrypted, ::TfheEncrypt, ::TfheDecrypt>;
class Tfhe$0 : public TfheBase$0 {
 public:
  Tfhe$0(const TFheGateBootstrappingParameterSet* params)
      : TfheBase$0(new_gate_bootstrapping_ciphertext_array(
                       Tfhe$0::element_bit_width(), params),
                   1, LweSampleArrayDeleter(Tfhe$0::element_bit_width())),
        params_(params) {}

  Tfhe$0& operator=(const Tfhe$0Ref rhs) {
    ::TfheCopy(rhs.get(), params_, this->get());
    return *this;
  }


  operator const Tfhe$0Ref() const& {
    return Tfhe$0Ref(const_cast<LweSample*>(get().data()), this->length());
  }
  operator Tfhe$0Ref() & {
    return Tfhe$0Ref(const_cast<LweSample*>(get().data()), this->length());
  }

  static Tfhe$0 Unencrypted(
      $1 value, const TFheGateBootstrappingCloudKeySet* key) {
    Tfhe$0 plaintext(key->params);
    plaintext.SetUnencrypted(value, key);
    return plaintext;
  }

  static Tfhe$0 Encrypt(
      $1 value, const TFheGateBootstrappingSecretKeySet* key) {
    Tfhe$0 ciphertext(key->params);
    ciphertext.SetEncrypted(value, key);
    return ciphertext;
  }
 private:
  const TFheGateBootstrappingParameterSet* params_;
};

template <unsigned... Dimensions>
using TfheBase$0Array = GenericEncoded$0Array<
    LweSample, LweSampleArrayDeleter, TFheGateBootstrappingSecretKeySet,
    TFheGateBootstrappingCloudKeySet, TFheGateBootstrappingParameterSet,
    ::TfheCopy, ::TfheUnencrypted, ::TfheEncrypt, ::TfheDecrypt, Dimensions...>;

template <unsigned... Dimensions>
class Tfhe$0ArrayRef;

template <unsigned... Dimensions>
class Tfhe$0Array;

template <>
class Tfhe$0Array<> : public TfheBase$0Array<> {
 public:
  Tfhe$0Array(size_t length, const TFheGateBootstrappingParameterSet* params)
      : TfheBase$0Array<>(
            new_gate_bootstrapping_ciphertext_array(
                Tfhe$0::element_bit_width() * length, params),
            length,
            LweSampleArrayDeleter(Tfhe$0::element_bit_width() * length)) {}
};

template <unsigned D1>
class Tfhe$0Array<D1> : public TfheBase$0Array<D1> {
 public:
  Tfhe$0Array(const TFheGateBootstrappingParameterSet* params)
      : TfheBase$0Array<D1>(
            new_gate_bootstrapping_ciphertext_array(
                Tfhe$0::element_bit_width() * D1, params),
            D1, LweSampleArrayDeleter(Tfhe$0::element_bit_width() * D1)) {}
};

template <unsigned... Dimensions>
class Tfhe$0Array : public TfheBase$0Array<Dimensions...> {
 public:
  Tfhe$0Array(const TFheGateBootstrappingParameterSet* params)
      : TfheBase$0Array<Dimensions...>(
            new_gate_bootstrapping_ciphertext_array(
                Tfhe$0::element_bit_width() *
                    TfheBase$0Array<Dimensions...>::VOLUME,
                params),
            TfheBase$0Array<Dimensions...>::VOLUME,
            LweSampleArrayDeleter(Tfhe$0::element_bit_width() *
                                  TfheBase$0Array<Dimensions...>::VOLUME)) {}
};

template <unsigned... Dimensions>
using TfheBase$0ArrayRef = GenericEncoded$0ArrayRef<
    LweSample, LweSampleArrayDeleter, TFheGateBootstrappingSecretKeySet,
    TFheGateBootstrappingCloudKeySet, TFheGateBootstrappingParameterSet,
    ::TfheCopy, ::TfheUnencrypted, ::TfheEncrypt, ::TfheDecrypt, Dimensions...>;

template <unsigned... Dimensions>
class Tfhe$0ArrayRef;

template <unsigned... Dimensions>
class Tfhe$0Array;

template <>
class Tfhe$0ArrayRef<> : public TfheBase$0ArrayRef<> {
 public:
  using TfheBase$0ArrayRef<>::TfheBase$0ArrayRef;
  Tfhe$0ArrayRef(const TfheBase$0ArrayRef<>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {
  }
  Tfhe$0ArrayRef(const TfheBase$0Array<>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {
  }

  using TfheBase$0ArrayRef<>::get;
};

template <unsigned D1>
class Tfhe$0ArrayRef<D1> : public TfheBase$0ArrayRef<D1> {
 public:
  using TfheBase$0ArrayRef<D1>::TfheBase$0ArrayRef;
  Tfhe$0ArrayRef(const TfheBase$0ArrayRef<D1>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {
  }
  Tfhe$0ArrayRef(const TfheBase$0Array<D1>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {
  }
  Tfhe$0ArrayRef(const TfheBase$0Array<>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {
    XLS_CHECK_GE(rhs.length(), D1);
  }
  using TfheBase$0ArrayRef<D1>::get;
};

template <unsigned... Dimensions>
class Tfhe$0ArrayRef : public TfheBase$0ArrayRef<Dimensions...> {
 public:
  using TfheBase$0ArrayRef<Dimensions...>::TfheBase$0ArrayRef;
  // Use the VOLUME constant instead of length().  The former gives the total
  // volume of the underlying array (accounting for all dimensions), while the
  // former gives the length of the first level of the array (e.g.,
  // Type[4][3][2] will have VOLUME = 24 and length() == 4.
  Tfhe$0ArrayRef(const TfheBase$0ArrayRef<Dimensions...>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()),
                       TfheBase$0ArrayRef<Dimensions...>::VOLUME) {}
  Tfhe$0ArrayRef(const TfheBase$0Array<Dimensions...>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()),
                       TfheBase$0Array<Dimensions...>::VOLUME) {}
  using TfheBase$0ArrayRef<Dimensions...>::get;
};

template <>
class TfheArrayRef<$1, typename std::enable_if_t<!std::is_integral_v<$1>>>: public Tfhe$0ArrayRef<> {
 public:
  using Tfhe$0ArrayRef<>::Tfhe$0ArrayRef;
  using Tfhe$0ArrayRef<>::get;
};

template <unsigned... Dimensions>
class TfheArrayRef<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, Dimensions...>: public Tfhe$0ArrayRef<Dimensions...> {
 public:
  using Tfhe$0ArrayRef<Dimensions...>::Tfhe$0ArrayRef;
  TfheArrayRef(const TfheArray<$1, std::enable_if_t<!std::is_integral_v<$1>>, Dimensions...>& rhs) : TfheArrayRef<$1, std::enable_if_t<!std::is_integral_v<$1>>, Dimensions...>(const_cast<LweSample*>(rhs.get().data()), Tfhe$0ArrayRef<Dimensions...>::VOLUME) {}
  using Tfhe$0ArrayRef<Dimensions...>::get;
};

template <>
class TfheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>>: public Tfhe$0Array<> {
 public:
  using Tfhe$0Array<>::Tfhe$0Array;
  using Tfhe$0Array<>::get;

  static TfheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>> Unencrypted(
     absl::Span<const $1> plaintext,
     const TFheGateBootstrappingCloudKeySet* key) {
    TfheArray<$1> shared_value(plaintext.length(), key->params);
    shared_value.SetUnencrypted(plaintext.data(), plaintext.length(), key);
    return shared_value;
  }

  static TfheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>> Encrypt(
      absl::Span<const $1> plaintext,
      const TFheGateBootstrappingSecretKeySet* key) {
    TfheArray<$1> private_value(plaintext.length(), key->params);
    private_value.SetEncrypted(plaintext.data(), plaintext.length(), key);
    return private_value;
  }
};

template <unsigned D1>
class TfheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1> : public Tfhe$0Array<D1> {
 public:
  using Tfhe$0Array<D1>::Tfhe$0Array;
  using Tfhe$0Array<D1>::get;

  static TfheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1> Unencrypted(
     absl::Span<const $1> plaintext,
     const TFheGateBootstrappingCloudKeySet* key) {
    XLS_CHECK_EQ(plaintext.length(), D1);
    TfheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1> shared_value(key->params);
    shared_value.SetUnencrypted(plaintext.data(), key);
    return shared_value;
  }

  static TfheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1> Encrypt(
      absl::Span<const $1> plaintext,
      const TFheGateBootstrappingSecretKeySet* key) {
    XLS_CHECK_EQ(plaintext.length(), D1);
    TfheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1> private_value(key->params);
    private_value.SetEncrypted(plaintext.data(), key);
    return private_value;
  }
};

)";

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
        absl::Substitute(kTfheStructTemplate, struct_name.name(),
                         struct_name.fully_qualified_name());
    generated.push_back(struct_text);
  }

  return absl::Substitute(kTfheFileTemplate, header_guard, generic_header,
                          absl::StrJoin(generated, "\n\n"));
}

// OpenFHE header template
//  0: Header guard
//  1: Generic header
//  2: Class definitions
constexpr const char kOpenFheFileTemplate[] = R"(#ifndef $0
#define $0

#include <memory>

#include "transpiler/data/openfhe_value.h"
#include "$1"
#include "absl/types/span.h"
#include "palisade/binfhe/binfhecontext.h"

$2
#endif//$0)";

// OpenFhe struct template
//  0: Type name
//  1: Fully-qualified type name
constexpr const char kOpenFheStructTemplate[] = R"(
using OpenFheBase$0Ref = GenericEncoded$0Ref<
    lbcrypto::LWECiphertext, std::default_delete<lbcrypto::LWECiphertext[]>,
    OpenFhePrivateKeySet, lbcrypto::BinFHEContext, void, ::OpenFheCopy,
    ::OpenFheUnencrypted, ::OpenFheEncrypt, ::OpenFheDecrypt>;
class OpenFhe$0Ref : public OpenFheBase$0Ref {
 public:
  OpenFhe$0Ref(lbcrypto::LWECiphertext* data, size_t length,
                lbcrypto::BinFHEContext cc)
      : OpenFheBase$0Ref(data, length), cc_(cc) {}
  OpenFhe$0Ref(const OpenFheBase$0Ref& rhs, lbcrypto::BinFHEContext cc)
      : OpenFhe$0Ref(const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
                      rhs.length(), cc) {}
  OpenFhe$0Ref(const OpenFhe$0Ref& rhs)
      : OpenFhe$0Ref(const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
                      rhs.length(), rhs.cc_) {}

  void SetEncrypted(const $1& value, lbcrypto::LWEPrivateKey sk,
                    size_t elem = 0) {
    OpenFhePrivateKeySet key{cc_, sk};
    OpenFheBase$0Ref::SetEncrypted(value, &key, elem);
  }

  $1 Decrypt(lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    return OpenFheBase$0Ref::Decrypt(&key, elem);
  }

 private:
  using OpenFheBase$0Ref::OpenFheBase$0Ref;
  lbcrypto::BinFHEContext cc_;
};

using OpenFheBase$0 = GenericEncoded$0<
    lbcrypto::LWECiphertext, std::default_delete<lbcrypto::LWECiphertext[]>,
    OpenFhePrivateKeySet, lbcrypto::BinFHEContext, void, ::OpenFheCopy,
    ::OpenFheUnencrypted, ::OpenFheEncrypt, ::OpenFheDecrypt>;
class OpenFhe$0 : public OpenFheBase$0 {
 public:
  OpenFhe$0(lbcrypto::BinFHEContext cc)
      : OpenFheBase$0(
            new lbcrypto::LWECiphertext[OpenFhe$0::element_bit_width()], 1,
            std::default_delete<lbcrypto::LWECiphertext[]>()),
        cc_(cc) {
    // Initialize the LWECiphertexts.
    SetUnencrypted({}, &cc_);
  }

  OpenFhe$0& operator=(const OpenFhe$0Ref rhs) {
    ::OpenFheCopy(rhs.get(), &cc_, this->get());
    return *this;
  }

  operator const OpenFhe$0Ref() const& {
    return OpenFhe$0Ref(const_cast<lbcrypto::LWECiphertext*>(get().data()),
                         this->length(), cc_);
  }
  operator OpenFhe$0Ref() & {
    return OpenFhe$0Ref(const_cast<lbcrypto::LWECiphertext*>(get().data()),
                         this->length(), cc_);
  }

  void SetEncrypted(const $1& value, lbcrypto::LWEPrivateKey sk,
                    size_t elem = 0) {
    OpenFhePrivateKeySet key{cc_, sk};
    OpenFheBase$0::SetEncrypted(value, &key, elem);
  }

  $1 Decrypt(lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    return OpenFheBase$0::Decrypt(&key, elem);
  }

  static OpenFhe$0 Encrypt(const $1& value, lbcrypto::BinFHEContext cc,
                    lbcrypto::LWEPrivateKey sk) {
    OpenFhe$0 val(cc);
    val.SetEncrypted(value, sk);
    return val;
  }

 private:
  lbcrypto::BinFHEContext cc_;
};

template <unsigned... Dimensions>
class OpenFhe$0Array;

template <unsigned... Dimensions>
class OpenFhe$0ArrayRef;

template <unsigned... Dimensions>
using OpenFheBase$0Array = GenericEncoded$0Array<
    lbcrypto::LWECiphertext, std::default_delete<lbcrypto::LWECiphertext[]>,
    OpenFhePrivateKeySet, lbcrypto::BinFHEContext, void, ::OpenFheCopy,
    ::OpenFheUnencrypted, ::OpenFheEncrypt, ::OpenFheDecrypt, Dimensions...>;

template <>
class OpenFhe$0Array<> : public OpenFheBase$0Array<> {
 public:
  OpenFhe$0Array(size_t length, lbcrypto::BinFHEContext cc)
      : OpenFheBase$0Array<>(
            new lbcrypto::LWECiphertext[OpenFhe$0::element_bit_width() *
                                        length],
            length, std::default_delete<lbcrypto::LWECiphertext[]>()),
        cc_(cc) {
    // Initialize the LWECiphertexts.
    for (int i = 0; i < length; i++) {
      OpenFheBase$0::SetUnencrypted({}, &cc_, i);
    }
  }

  void SetEncrypted(const $1* value, size_t length,
                    lbcrypto::LWEPrivateKey sk) {
    OpenFhePrivateKeySet key{cc_, sk};
    OpenFheBase$0Array<>::SetEncrypted(value, length, &key);
  }

  void SetEncrypted(absl::Span<const $1> value, lbcrypto::LWEPrivateKey sk) {
    SetEncrypted(value.data(), value.size(), sk);
  }

  void Decrypt($1* value, size_t length, lbcrypto::LWEPrivateKey sk) const {
    OpenFhePrivateKeySet key{cc_, sk};
    OpenFheBase$0Array<>::Decrypt(value, length, &key);
  }

  absl::FixedArray<$1> Decrypt(lbcrypto::LWEPrivateKey sk) const {
    OpenFhePrivateKeySet key{cc_, sk};
    return OpenFheBase$0Array<>::Decrypt(&key);
  }

  OpenFhe$0Ref operator[](size_t pos) {
    auto ref = this->OpenFheBase$0Array<>::operator[](pos);
    return OpenFhe$0Ref(ref, cc_);
  }

 private:
  friend OpenFhe$0ArrayRef<>;
  lbcrypto::BinFHEContext cc_;
};

template <unsigned D1>
class OpenFhe$0Array<D1> : public OpenFheBase$0Array<D1> {
 public:
  OpenFhe$0Array(lbcrypto::BinFHEContext cc)
      : OpenFheBase$0Array<D1>(
            new lbcrypto::LWECiphertext[OpenFhe$0::element_bit_width() * D1],
            D1, std::default_delete<lbcrypto::LWECiphertext[]>()),
        cc_(cc) {
    // Initialize the LWECiphertexts.
    for (int i = 0; i < D1; i++) {
      OpenFheBase$0::SetUnencrypted({}, &cc_, i);
    }
  }

  void SetEncrypted(const typename OpenFheBase$0Array<D1>::ArrayT value,
                    lbcrypto::LWEPrivateKey sk, size_t elem = 0) {
    OpenFhePrivateKeySet key{cc_, sk};
    OpenFheBase$0Array<D1>::SetEncrypted(value, &key, elem);
  }

  void SetEncrypted(absl::Span<const $1> value, lbcrypto::LWEPrivateKey sk) {
    XLS_CHECK_EQ(value.size(), D1);
    SetEncrypted(value.data(), value.size(), sk);
  }

  void Decrypt(typename OpenFheBase$0Array<D1>::ArrayT value,
               lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    OpenFheBase$0Array<D1>::Decrypt(value, &key, elem);
  }

  absl::FixedArray<$1> Decrypt(lbcrypto::LWEPrivateKey sk) const {
    OpenFhePrivateKeySet key{cc_, sk};
    return OpenFheBase$0Array<D1>::Decrypt(&key);
  }

  OpenFhe$0Ref operator[](size_t pos) {
    auto ref = this->OpenFheBase$0Array<D1>::operator[](pos);
    return OpenFhe$0Ref(ref, cc_);
  }

 private:
  friend OpenFhe$0ArrayRef<D1>;
  lbcrypto::BinFHEContext cc_;
};

template <unsigned D1, unsigned... Dimensions>
class OpenFhe$0Array<D1, Dimensions...>
    : public OpenFheBase$0Array<D1, Dimensions...> {
 public:
  OpenFhe$0Array(lbcrypto::BinFHEContext cc)
      : OpenFheBase$0Array<D1, Dimensions...>(
            new lbcrypto::LWECiphertext
                [OpenFhe$0::element_bit_width() *
                 OpenFheBase$0Array<D1, Dimensions...>::VOLUME],
            OpenFheBase$0Array<D1, Dimensions...>::VOLUME,
            std::default_delete<lbcrypto::LWECiphertext[]>()),
        cc_(cc) {
    // Initialize the LWECiphertexts.
    for (int i = 0; i < OpenFheBase$0Array<D1, Dimensions...>::VOLUME; i++) {
      OpenFheBase$0::SetUnencrypted({}, &cc_, i);
    }
  }

  void SetEncrypted(
      const typename OpenFheBase$0Array<D1, Dimensions...>::ArrayT value,
      lbcrypto::LWEPrivateKey sk, size_t elem = 0) {
    OpenFhePrivateKeySet key{cc_, sk};
    OpenFheBase$0Array<D1, Dimensions...>::SetEncrypted(value, &key, elem);
  }

  void Decrypt(typename OpenFheBase$0Array<D1, Dimensions...>::ArrayT value,
               lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    OpenFheBase$0Array<D1, Dimensions...>::Decrypt(value, &key, elem);
  }

  OpenFhe$0ArrayRef<Dimensions...> operator[](size_t pos) {
    auto ref = this->OpenFheBase$0Array<D1, Dimensions...>::operator[](pos);
    return OpenFhe$0ArrayRef<Dimensions...>(ref, cc_);
  }

 private:
  friend OpenFhe$0ArrayRef<D1, Dimensions...>;
  lbcrypto::BinFHEContext cc_;
};

template <unsigned... Dimensions>
using OpenFheBase$0ArrayRef = GenericEncoded$0ArrayRef<
    lbcrypto::LWECiphertext, std::default_delete<lbcrypto::LWECiphertext[]>,
    OpenFhePrivateKeySet, lbcrypto::BinFHEContext, void, ::OpenFheCopy,
    ::OpenFheUnencrypted, ::OpenFheEncrypt, ::OpenFheDecrypt, Dimensions...>;

template <>
class OpenFhe$0ArrayRef<> : public OpenFheBase$0ArrayRef<> {
 public:
  OpenFhe$0ArrayRef(const OpenFhe$0ArrayRef<>& rhs)
      : OpenFheBase$0ArrayRef<>(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            rhs.length()) {
    cc_ = rhs.cc_;
  }
  OpenFhe$0ArrayRef(const OpenFhe$0Array<>& rhs)
      : OpenFheBase$0ArrayRef<>(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            rhs.length()) {
    cc_ = rhs.cc_;
  }

  OpenFhe$0Ref operator[](size_t pos) {
    auto ref = this->OpenFheBase$0ArrayRef<>::operator[](pos);
    return OpenFhe$0Ref(ref, cc_);
  }

 private:
  lbcrypto::BinFHEContext cc_;
};

template <unsigned D1>
class OpenFhe$0ArrayRef<D1> : public OpenFheBase$0ArrayRef<D1> {
 public:
  using OpenFheBase$0ArrayRef<D1>::OpenFheBase$0ArrayRef;
  OpenFhe$0ArrayRef(const OpenFhe$0ArrayRef<D1>& rhs)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            rhs.length()) {
    cc_ = rhs.cc_;
  }
  OpenFhe$0ArrayRef(const OpenFhe$0Array<D1>& rhs)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            rhs.length()) {
    cc_ = rhs.cc_;
  }
  OpenFhe$0ArrayRef(const OpenFhe$0Array<>& rhs)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            rhs.length()) {
    XLS_CHECK_GE(rhs.length(), D1);
    cc_ = rhs.cc_;
  }
  OpenFhe$0ArrayRef(const OpenFheBase$0ArrayRef<D1>& rhs,
                     lbcrypto::BinFHEContext cc)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            OpenFheBase$0ArrayRef<D1>::VOLUME) {
    cc_ = cc;
  }
  OpenFhe$0ArrayRef(absl::Span<const lbcrypto::LWECiphertext> data,
                     lbcrypto::BinFHEContext cc)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(data.data()),
            OpenFheBase$0ArrayRef<D1>::VOLUME) {
    XLS_CHECK_EQ(data.length(), this->bit_width());
    cc_ = cc;
  }

  OpenFhe$0Ref operator[](size_t pos) {
    auto ref = this->OpenFheBase$0ArrayRef<D1>::operator[](pos);
    return OpenFhe$0Ref(ref, cc_);
  }

  void SetEncrypted(const typename OpenFheBase$0ArrayRef<D1>::ArrayT value,
                    lbcrypto::LWEPrivateKey sk, size_t elem = 0) {
    OpenFhePrivateKeySet key{cc_, sk};
    this->OpenFheBase$0ArrayRef<D1>::SetEncrypted(value, &key, elem);
  }

  void Decrypt(typename OpenFheBase$0ArrayRef<D1>::ArrayT value,
               lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    this->OpenFheBase$0ArrayRef<D1>::Decrypt(value, &key, elem);
  }

  absl::FixedArray<$1> Decrypt(lbcrypto::LWEPrivateKey sk) const {
    OpenFhePrivateKeySet key{cc_, sk};
    return this->OpenFheBase$0ArrayRef<>::Decrypt(&key);
  }

  lbcrypto::BinFHEContext cc() { return cc_; }
 private:
  lbcrypto::BinFHEContext cc_;
};

template <unsigned D1, unsigned... Dimensions>
class OpenFhe$0ArrayRef<D1, Dimensions...>
    : public OpenFheBase$0ArrayRef<D1, Dimensions...> {
 public:
  using OpenFheBase$0ArrayRef<D1, Dimensions...>::OpenFheBase$0ArrayRef;
  // Use the VOLUME constant instead of length().  The former gives the total
  // volume of the underlying array (accounting for all dimensions), while the
  // former gives the length of the first level of the array (e.g.,
  // Type[4][3][2] will have VOLUME = 24 and length() == 4.
  OpenFhe$0ArrayRef(const OpenFhe$0ArrayRef<D1, Dimensions...>& rhs)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            OpenFheBase$0ArrayRef<D1, Dimensions...>::VOLUME) {
    cc_ = rhs.cc_;
  }
  OpenFhe$0ArrayRef(const OpenFhe$0Array<D1, Dimensions...>& rhs)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            OpenFheBase$0ArrayRef<D1, Dimensions...>::VOLUME) {
    cc_ = rhs.cc_;
  }
  OpenFhe$0ArrayRef(const OpenFheBase$0ArrayRef<D1, Dimensions...>& rhs,
                     lbcrypto::BinFHEContext cc)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            OpenFheBase$0ArrayRef<D1, Dimensions...>::VOLUME) {
    cc_ = cc;
  }
  OpenFhe$0ArrayRef(absl::Span<const lbcrypto::LWECiphertext> data,
                     lbcrypto::BinFHEContext cc)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(data.data()),
            OpenFheBase$0ArrayRef<D1, Dimensions...>::VOLUME) {
    XLS_CHECK_EQ(data.length(), this->bit_width());
    cc_ = cc;
  }

  OpenFhe$0ArrayRef<Dimensions...> operator[](size_t pos) {
    auto ref = this->OpenFheBase$0ArrayRef<D1, Dimensions...>::operator[](pos);
    return OpenFhe$0ArrayRef<Dimensions...>(ref, cc_);
  }

  void SetEncrypted(
      const typename OpenFheBase$0ArrayRef<D1, Dimensions...>::ArrayT value,
      lbcrypto::LWEPrivateKey sk, size_t elem = 0) {
    OpenFhePrivateKeySet key{cc_, sk};
    this->OpenFheBase$0ArrayRef<D1, Dimensions...>::SetEncrypted(value, &key,
                                                                  elem);
  }

  void Decrypt(typename OpenFheBase$0ArrayRef<D1, Dimensions...>::ArrayT value,
               lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    this->OpenFheBase$0ArrayRef<D1, Dimensions...>::Decrypt(value, &key, elem);
  }

  lbcrypto::BinFHEContext cc() { return cc_; }
 private:
  lbcrypto::BinFHEContext cc_;
};

template <>
class OpenFheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>>: public OpenFhe$0Array<> {
 public:
  using OpenFhe$0Array<>::OpenFhe$0Array;

  static OpenFheArray<$1> Encrypt(
      absl::Span<const $1> plaintext, lbcrypto::BinFHEContext cc,
      lbcrypto::LWEPrivateKey sk) {
    OpenFheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>> private_value(plaintext.length(), cc);
    private_value.SetEncrypted(plaintext.data(), plaintext.length(), sk);
    return private_value;
  }
};

template <unsigned D1>
class OpenFheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1> : public OpenFhe$0Array<D1> {
 public:
  using OpenFhe$0Array<D1>::OpenFhe$0Array;

  static OpenFheArray<$1> Encrypt(
      absl::Span<const $1> plaintext, lbcrypto::BinFHEContext cc,
      lbcrypto::LWEPrivateKey sk) {
    OpenFheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1> private_value(plaintext.length(), cc);
    private_value.SetEncrypted(plaintext.data(), plaintext.length(), sk);
    return private_value;
  }
};

template <unsigned D1, unsigned... Dimensions>
class OpenFheArray<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1, Dimensions...> : public OpenFhe$0Array<D1, Dimensions...> {
 public:
  using OpenFhe$0Array<D1, Dimensions...>::OpenFhe$0Array;

  OpenFheArrayRef<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, Dimensions...> operator[](size_t pos) {
    auto ref = this->OpenFhe$0Array<D1, Dimensions...>::operator[](pos);
    return OpenFheArrayRef<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, Dimensions...>(ref.get(), ref.cc());
  }
};

template <>
class OpenFheArrayRef<$1, typename std::enable_if_t<!std::is_integral_v<$1>>>: public OpenFhe$0ArrayRef<> {
public:
  using OpenFhe$0ArrayRef<>::OpenFhe$0ArrayRef;
};

template <unsigned D1, unsigned... Dimensions>
class OpenFheArrayRef<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, D1, Dimensions...>
    : public OpenFhe$0ArrayRef<D1, Dimensions...> {
 public:
  using OpenFhe$0ArrayRef<D1, Dimensions...>::OpenFhe$0ArrayRef;

  OpenFheArrayRef<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, Dimensions...> operator[](size_t pos) {
    auto ref = this->OpenFhe$0ArrayRef<D1, Dimensions...>::operator[](pos);
    return OpenFheArrayRef<$1, typename std::enable_if_t<!std::is_integral_v<$1>>, Dimensions...>(ref.get(), ref.cc());
  }
};
)";

absl::StatusOr<std::string> ConvertStructsToEncodedOpenFhe(
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
        absl::Substitute(kOpenFheStructTemplate, struct_name.name(),
                         struct_name.fully_qualified_name());
    generated.push_back(struct_text);
  }

  return absl::Substitute(kOpenFheFileTemplate, header_guard, generic_header,
                          absl::StrJoin(generated, "\n\n"));
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
