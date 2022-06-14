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
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "transpiler/common_transpiler.h"
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
    const IdToType& id_to_type, const StructField& field, bool encrypt,
    bool use_field = true) {
  std::vector<std::string> lines;
  std::string field_name = "value";
  if (use_field) {
    absl::StrAppend(&field_name, ".", field.name());
  }
  if (field.type().has_as_array()) {
    XLS_ASSIGN_OR_RETURN(
        std::string array_lines,
        GenerateSetOrEncryptArrayElement(id_to_type, field_name,
                                         field.type().as_array(), encrypt));
    lines.push_back(array_lines);
  } else if (field.type().has_as_bool()) {
    XLS_ASSIGN_OR_RETURN(std::string bool_lines,
                         GenerateSetOrEncryptBoolElement(field_name, encrypt));
    lines.push_back(bool_lines);
  } else if (field.type().has_as_int()) {
    XLS_ASSIGN_OR_RETURN(
        std::string int_lines,
        GenerateSetOrEncryptIntegralElement(field.type(), field_name, encrypt));
    lines.push_back(int_lines);
  } else if (field.type().has_as_inst()) {
    XLS_ASSIGN_OR_RETURN(
        std::string struct_lines,
        GenerateSetOrEncryptStructElement(id_to_type, field.type().as_inst(),
                                          field_name, encrypt));
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
  lines.push_back(absl::Substitute(
      "        $0(EncodedValue<bool>($1).get(), key, absl::MakeSpan(data, 1));",
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

  lines.push_back(absl::Substitute(
      "        $0(EncodedValue<$1>($2).get(), key, absl::MakeSpan(data, $3));",
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
  lines.push_back(absl::Substitute(
      "        GenericEncoded<$2, Sample, SampleArrayDeleter, SecretKey, "
      "PublicKey, BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, "
      "DecryptFn>::Borrowed$0($1, data, key);",
      op, source_var, instance_type.name().name()));
  lines.push_back(absl::Substitute("        data += $0;", type_data.bit_width));

  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateSetOrEncryptFunction(
    const IdToType& id_to_type, const StructType& struct_type, bool encrypt,
    bool use_field) {
  std::vector<std::string> lines;
  for (int i = 0; i < struct_type.fields_size(); i++) {
    XLS_ASSIGN_OR_RETURN(
        std::string line,
        GenerateSetOrEncryptOneElement(id_to_type, struct_type.fields(i),
                                       encrypt, use_field));
    lines.push_back(line);
  }
  std::vector<std::string> reversed_buffer_lines = lines;
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
  lines.push_back(absl::Substitute("        $0 = encoded_$1.Decode();",
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
  lines.push_back(absl::Substitute("        $0 = encoded_$1.Decode();",
                                   output_loc, temp_name));
  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateDecryptStruct(
    const IdToType& id_to_type, const InstanceType& instance_type,
    absl::string_view output_loc) {
  std::vector<std::string> lines;
  TypeData type_data = id_to_type.at(instance_type.name().id());
  lines.push_back(absl::Substitute(
      "        GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, "
      "PublicKey, BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, "
      "DecryptFn>::BorrowedDecrypt(data, &$1, key);",
      instance_type.name().name(), output_loc));
  lines.push_back(absl::Substitute("        data += $0;", type_data.bit_width));
  return absl::StrJoin(lines, "\n");
}

absl::StatusOr<std::string> GenerateDecryptFunction(
    const IdToType& id_to_type, const StructType& struct_type,
    bool use_field = true) {
  std::vector<std::string> lines;
  std::vector<std::string> reversed_buffer_lines;
  for (int i = 0; i < struct_type.fields_size(); i++) {
    const StructField& field = struct_type.fields(i);
    std::string field_name = struct_type.fields(i).name();
    std::string output_loc;
    if (use_field) {
      output_loc = absl::StrCat("result->", field_name);
    } else {
      output_loc = "(*result)";
    }
    XLS_ASSIGN_OR_RETURN(std::string line, GenerateDecryptOneElement(
                                               id_to_type, field, output_loc));
    lines.push_back(line);
  }
  reversed_buffer_lines = lines;
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
#include "transpiler/data/generic_value.h"
$0

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
template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          CopyFnT<$0, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<$0, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<$0, Sample, SecretKey> EncryptFn,
          DecryptFnT<$0, Sample, SecretKey> DecryptFn>
class GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                        BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                        DecryptFn> {
 public:
  GenericEncodedRef(Sample* data, size_t length)
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
          CopyFnT<$0, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<$0, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<$0, Sample, SecretKey> EncryptFn,
          DecryptFnT<$0, Sample, SecretKey> DecryptFn>
class GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                     BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                     DecryptFn> {
 public:
  GenericEncoded(const $1& value, const PublicKey* key) {
    SetUnencrypted(value, key);
  }

  GenericEncoded(Sample* data, size_t length, SampleArrayDeleter deleter)
      : length_(length), data_(data, deleter) {}
  GenericEncoded(GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                     BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                     DecryptFn>&&) = default;

  operator const GenericEncodedRef<
      $0, Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn>() const& {
    return GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey,
                             PublicKey, BootstrappingKey, CopyFn, UnencryptedFn,
                             EncryptFn, DecryptFn>(data_.get(), this->length());
  }
  operator GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey,
                             PublicKey, BootstrappingKey, CopyFn, UnencryptedFn,
                             EncryptFn, DecryptFn>() & {
    return GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey,
                             PublicKey, BootstrappingKey, CopyFn, UnencryptedFn,
                             EncryptFn, DecryptFn>(data_.get(), this->length());
  }

  // We set values here directly, instead of using EncodedValue, since
  // EncodedValue types own their arrays, whereas we'd need to own them here as
  // contiguously-allocated chunks. (We could modify them to use borrowed data,
  // but it'd be more work than this). For structure types, though, we do
  // enable setting on "borrowed" data to enable recursive setting.
  void SetUnencrypted(const $1& value, const PublicKey* key, size_t elem = 0) {
    SetUnencryptedInternal(value, key,
                           data_.get() + elem * element_bit_width());
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
          CopyFnT<$0, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<$0, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<$0, Sample, SecretKey> EncryptFn,
          DecryptFnT<$0, Sample, SecretKey> DecryptFn>
class GenericEncodedArray<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn>
    : public GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                              BootstrappingKey, CopyFn, UnencryptedFn,
                              EncryptFn, DecryptFn> {
 public:
  using GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                         BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                         DecryptFn>::GenericEncoded;

  operator const GenericEncodedArrayRef<$0,
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn>() const& {
    return GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn>(
        this->get(), this->length());
  }
  operator GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn>() & {
    return GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn>(
        this->get(), this->length());
  }

  template <unsigned D1>
  operator const GenericEncodedArrayRef<$0,
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1>() const& {
    return GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn, D1>(
        this->get(), this->length());
  }
  template <unsigned D1>
  operator GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn, D1>() & {
    return GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey,
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
      GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                       BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                       DecryptFn>::SetUnencrypted(value[i], key, i);
    }
  }

  void SetEncrypted(const $1* value, size_t len, const SecretKey* key) {
    XLS_CHECK(this->length() >= len);
    for (size_t i = 0; i < len; i++) {
      GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                       BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                       DecryptFn>::SetEncrypted(value[i], key, i);
    }
  }

  void SetEncrypted(absl::Span<const $1> values, const SecretKey* key) {
    XLS_CHECK(this->length() >= values.size());
    for (size_t i = 0; i < values.size(); i++) {
      GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                       BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                       DecryptFn>::SetEncrypted(values[i], key, i);
    }
  }

  void Decrypt($1* result, size_t len, const SecretKey* key) const {
    XLS_CHECK(len >= this->length());
    for (size_t i = 0; i < this->length(); i++) {
      result[i] =
          GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                           BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                           DecryptFn>::Decrypt(key, i);
    }
  }

  absl::FixedArray<$1> Decrypt(const SecretKey* key) const {
    absl::FixedArray<$1> plaintext(this->length());
    Decrypt(plaintext.data(), this->length(), key);
    return plaintext;
  }

  using GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                         BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn>::get;

  GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                      BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                      DecryptFn>
  operator[](size_t pos) {
    XLS_CHECK(pos < this->length());
    auto span = this->get();
    return GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>(span.data() + pos * $5, 1);
  }
};

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          CopyFnT<$0, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<$0, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<$0, Sample, SecretKey> EncryptFn,
          DecryptFnT<$0, Sample, SecretKey> DecryptFn, unsigned D1>
class GenericEncodedArray<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, D1>
    : public GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                              BootstrappingKey, CopyFn, UnencryptedFn,
                              EncryptFn, DecryptFn> {
 public:
  using GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                         BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                         DecryptFn>::GenericEncoded;
  enum { VOLUME = D1 };
  using ArrayT = $1[D1];

  operator const GenericEncodedArrayRef<$0,
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1>() const& {
    return GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn, D1>(
        this->get(), this->length());
  }
  operator GenericEncodedArrayRef<$0,
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1>() & {
    return GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey,
                                    PublicKey, BootstrappingKey, CopyFn,
                                    UnencryptedFn, EncryptFn, DecryptFn, D1>(
        this->get(), this->length());
  }

  void SetUnencrypted(const ArrayT value, const PublicKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                       BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                       DecryptFn>::SetUnencrypted(value[i], key, elem * D1 + i);
    }
  }

  void SetEncrypted(const ArrayT value, const SecretKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                       BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                       DecryptFn>::SetEncrypted(value[i], key, elem * D1 + i);
    }
  }

  void Decrypt(ArrayT result, const SecretKey* key, size_t elem = 0) const {
    for (size_t i = 0; i < D1; i++) {
      result[i] =
          GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                           BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                           DecryptFn>::Decrypt(key, elem * D1 + i);
    }
  }

  absl::FixedArray<$1> Decrypt(const SecretKey* key) const {
    absl::FixedArray<$1> plaintext(this->length());
    Decrypt(plaintext.data(), key);
    return plaintext;
  }

  using GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                         BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn>::get;

  GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                      BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                      DecryptFn>
  operator[](size_t pos) {
    XLS_CHECK(pos < this->length());
    auto span = this->get();
    return GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>(span.data() + pos * $5, 1);
  }
  // The base will return the full length of the underlying array, when we want
  // to return the length of this level of the multi-dimensional array.
  size_t length() const { return D1; }
};

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          CopyFnT<$0, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<$0, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<$0, Sample, SecretKey> EncryptFn,
          DecryptFnT<$0, Sample, SecretKey> DecryptFn, unsigned D1,
          unsigned... Dimensions>
class GenericEncodedArray<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, D1, Dimensions...>
    : public GenericEncodedArray<$0,
          Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
          CopyFn, UnencryptedFn, EncryptFn, DecryptFn, Dimensions...> {
 public:
  using GenericEncodedArray<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                              BootstrappingKey, CopyFn, UnencryptedFn,
                              EncryptFn, DecryptFn,
                              Dimensions...>::GenericEncodedArray;
  using LowerT =
      GenericEncodedArray<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, Dimensions...>;
  using LowerArrayT = typename LowerT::ArrayT;
  enum { VOLUME = D1 * LowerT::VOLUME };
  using ArrayT = LowerArrayT[D1];

  operator const GenericEncodedArrayRef<$0,
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1, Dimensions...>() const& {
    return GenericEncodedArrayRef<$0,
        Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
        CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1, Dimensions...>(
        this->get(), this->length());
  }
  operator GenericEncodedArrayRef<$0,
      Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
      CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1, Dimensions...>() & {
    return GenericEncodedArrayRef<$0,
        Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
        CopyFn, UnencryptedFn, EncryptFn, DecryptFn, D1, Dimensions...>(
        this->get(), this->length());
  }

  void SetUnencrypted(const ArrayT value, const PublicKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncodedArray<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn,
                            Dimensions...>::SetUnencrypted(value[i], key,
                                                           elem * D1 + i);
    }
  }

  void SetEncrypted(const ArrayT value, const SecretKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncodedArray<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, Dimensions...>::SetEncrypted(value[i],
                                                                    key,
                                                                    elem * D1 +
                                                                        i);
    }
  }

  void Decrypt(ArrayT result, const SecretKey* key, size_t elem = 0) const {
    for (size_t i = 0; i < D1; i++) {
      GenericEncodedArray<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, Dimensions...>::Decrypt(result[i], key,
                                                               elem * D1 + i);
    }
  }

  using GenericEncoded<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                         BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn>::get;

  GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                           BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                           DecryptFn, Dimensions...>
  operator[](size_t pos) {
    XLS_CHECK(pos < this->length());
    auto span = this->get();
    return GenericEncodedArrayRef<$0,
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
          CopyFnT<$0, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<$0, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<$0, Sample, SecretKey> EncryptFn,
          DecryptFnT<$0, Sample, SecretKey> DecryptFn>
class GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>
    : public GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey,
                                 PublicKey, BootstrappingKey, CopyFn,
                                 UnencryptedFn, EncryptFn, DecryptFn> {
 public:
  GenericEncodedArrayRef(Sample* data, size_t length)
      : GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn>(data, length) {}

  void SetUnencrypted(const $1* value, size_t len, const SecretKey* key) {
    XLS_CHECK(this->length() >= len);
    for (size_t i = 0; i < len; i++) {
      GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                          BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                          DecryptFn>::SetUnencrypted(value[i], key, i);
    }
  }

  void SetEncrypted(const $1* value, size_t len, const SecretKey* key) {
    XLS_CHECK(this->length() >= len);
    for (size_t i = 0; i < len; i++) {
      GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                          BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                          DecryptFn>::SetEncrypted(value[i], key, i);
    }
  }

  void Decrypt($1* result, size_t len, const SecretKey* key) const {
    XLS_CHECK(len >= this->length());
    for (size_t i = 0; i < this->length(); i++) {
      result[i] =
          GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                              BootstrappingKey, CopyFn, UnencryptedFn,
                              EncryptFn, DecryptFn>::Decrypt(key, i);
    }
  }

  absl::FixedArray<$1> Decrypt(const SecretKey* key) const {
    absl::FixedArray<$1> plaintext(this->length());
    Decrypt(plaintext.data(), this->length(), key);
    return plaintext;
  }

  using GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey,
                            PublicKey, BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn>::get;

  GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                      BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                      DecryptFn>
  operator[](size_t pos) {
    XLS_CHECK(pos < this->length());
    auto span = this->get();
    return GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>(span.data() + pos * $5, 1);
  }
};

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          CopyFnT<$0, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<$0, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<$0, Sample, SecretKey> EncryptFn,
          DecryptFnT<$0, Sample, SecretKey> DecryptFn, unsigned D1>
class GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn, D1>
    : public GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey,
                                 PublicKey, BootstrappingKey, CopyFn,
                                 UnencryptedFn, EncryptFn, DecryptFn> {
 public:
  enum { VOLUME = D1 };
  using ArrayT = $1[D1];

  GenericEncodedArrayRef(Sample* data, size_t length)
      : GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn>(data, length) {}

  void SetUnencrypted(const ArrayT value, const SecretKey* key, size_t elem) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                          BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                          DecryptFn>::SetUnencrypted(value[i], key,
                                                     elem * D1 + i);
    }
  }

  void SetEncrypted(const ArrayT value, const SecretKey* key, size_t elem) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                          BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                          DecryptFn>::SetEncrypted(value[i], key,
                                                   elem * D1 + i);
    }
  }

  void Decrypt(ArrayT result, const SecretKey* key, size_t elem = 0) const {
    for (size_t i = 0; i < D1; i++) {
      result[i] =
          GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                              BootstrappingKey, CopyFn, UnencryptedFn,
                              EncryptFn, DecryptFn>::Decrypt(key,
                                                             elem * D1 + i);
    }
  }

  absl::FixedArray<$1> Decrypt(const SecretKey* key) const {
    absl::FixedArray<$1> plaintext(this->length());
    Decrypt(plaintext.data(), key);
    return plaintext;
  }

  using GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey,
                            PublicKey, BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn>::get;

  GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                      BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                      DecryptFn>
  operator[](size_t pos) {
    XLS_CHECK(pos < this->length());
    auto span = this->get();
    return GenericEncodedRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn>(span.data() + pos * $5, 1);
  }

  // The base will return the full length of the underlying array, when we want
  // to return the length of this level of the multi-dimensional array.
  size_t length() const { return D1; }
};

template <class Sample, class SampleArrayDeleter, class SecretKey,
          class PublicKey, class BootstrappingKey,
          CopyFnT<$0, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<$0, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<$0, Sample, SecretKey> EncryptFn,
          DecryptFnT<$0, Sample, SecretKey> DecryptFn, unsigned D1,
          unsigned... Dimensions>
class GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn, D1, Dimensions...>
    : public GenericEncodedArrayRef<$0,
          Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
          CopyFn, UnencryptedFn, EncryptFn, DecryptFn, Dimensions...> {
 public:
  using LowerT =
      GenericEncodedArray<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                            BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                            DecryptFn, Dimensions...>;
  using LowerArrayT = typename LowerT::ArrayT;
  enum { VOLUME = D1 * LowerT::VOLUME };
  using ArrayT = LowerArrayT[D1];

  GenericEncodedArrayRef(Sample* data, size_t length)
      : GenericEncodedArrayRef<$0,
            Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
            CopyFn, UnencryptedFn, EncryptFn, DecryptFn, Dimensions...>(
            data, length) {}

  void SetUnencrypted(const ArrayT value, const SecretKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn,
                               Dimensions...>::SetUnencrypted(value[i], key,
                                                              elem * D1 + i);
    }
  }

  void SetEncrypted(const ArrayT value, const SecretKey* key, size_t elem = 0) {
    for (size_t i = 0; i < D1; i++) {
      GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn,
                               Dimensions...>::SetEncrypted(value[i], key,
                                                            elem * D1 + i);
    }
  }

  void Decrypt(ArrayT result, const SecretKey* key, size_t elem = 0) const {
    for (size_t i = 0; i < D1; i++) {
      GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                               BootstrappingKey, CopyFn, UnencryptedFn,
                               EncryptFn, DecryptFn,
                               Dimensions...>::Decrypt(result[i], key,
                                                       elem * D1 + i);
    }
  }

  using GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey,
                            PublicKey, BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn, DecryptFn,
                            Dimensions...>::get;

  GenericEncodedArrayRef<$0, Sample, SampleArrayDeleter, SecretKey, PublicKey,
                           BootstrappingKey, CopyFn, UnencryptedFn, EncryptFn,
                           DecryptFn, Dimensions...>
  operator[](size_t pos) {
    XLS_CHECK(pos < this->length());
    auto span = this->get();
    return GenericEncodedArrayRef<$0,
        Sample, SampleArrayDeleter, SecretKey, PublicKey, BootstrappingKey,
        CopyFn, UnencryptedFn, EncryptFn, DecryptFn, Dimensions...>(
        span.data() + pos * LowerT::VOLUME * $5, D1);
  }
  // The base will return the full length of the underlying array, when we want
  // to return the length of this level of the multi-dimensional array.
  size_t length() const { return D1; }
};
)";

absl::StatusOr<std::string> ConvertStructToEncoded(
    const IdToType& id_to_type, int64_t id,
    const std::vector<std::string>& unwrap) {
  const StructType& struct_type = id_to_type.at(id).type;
  xlscc_metadata::CPPName struct_name = struct_type.name().as_inst().name();
  bool use_field = true;
  std::string fully_qualified_name = struct_name.fully_qualified_name();
  if (std::find(unwrap.begin(), unwrap.end(), struct_name.name()) !=
      unwrap.end()) {
    if (struct_type.fields_size() != 1) {
      return absl::InvalidArgumentError(
          absl::StrFormat("Cannot unwrap struct %s, it has %d elements.",
                          struct_name.name(), struct_type.fields_size()));
    }
    const StructField& field = struct_type.fields().Get(0);
    fully_qualified_name = GetTypeName(field.type()).value();
    use_field = false;
  }

  XLS_ASSIGN_OR_RETURN(std::string set_fn, GenerateSetOrEncryptFunction(
                                               id_to_type, struct_type,
                                               /*encrypt=*/false, use_field));
  XLS_ASSIGN_OR_RETURN(
      std::string encrypt_fn,
      GenerateSetOrEncryptFunction(id_to_type, struct_type,
                                   /*encrypt=*/true, use_field));
  XLS_ASSIGN_OR_RETURN(
      std::string decrypt_fn,
      GenerateDecryptFunction(id_to_type, struct_type, use_field));
  int64_t bit_width = GetStructWidth(id_to_type, struct_type);
  std::string result =
      absl::Substitute(kClassTemplate, struct_name.name(), fully_qualified_name,
                       set_fn, encrypt_fn, decrypt_fn, bit_width);
  return result;
}

}  // namespace

absl::StatusOr<std::string> ConvertStructsToEncodedTemplate(
    const xlscc_metadata::MetadataOutput& metadata,
    const std::vector<std::string>& original_headers,
    absl::string_view output_path, const std::vector<std::string>& unwrap) {
  if (metadata.structs_size() == 0) {
    return "";
  }

  std::string header_guard = GenerateHeaderGuard(output_path);

  std::vector<int64_t> struct_order = GetTypeReferenceOrder(metadata);
  IdToType id_to_type = PopulateTypeData(metadata, struct_order);
  std::vector<std::string> generated;
  for (int64_t id : struct_order) {
    XLS_ASSIGN_OR_RETURN(std::string struct_text,
                         ConvertStructToEncoded(id_to_type, id, unwrap));
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

template <typename T>
using __EncodedBaseRef =
    GenericEncodedRef<T, bool, std::default_delete<bool[]>, void, void, void,
                        ::CleartextCopy, ::CleartextEncode, ::CleartextEncode,
                        ::CleartextDecode>;

template <typename T>
using __EncodedBase =
    GenericEncoded<T, bool, std::default_delete<bool[]>, void, void, void,
                     ::CleartextCopy, ::CleartextEncode, ::CleartextEncode,
                     ::CleartextDecode>;

template <typename T, unsigned... Dimensions>
using __EncodedBaseArrayRef = GenericEncodedArrayRef<T,
    bool, std::default_delete<bool[]>, void, void, void, ::CleartextCopy,
    ::CleartextEncode, ::CleartextEncode, ::CleartextDecode, Dimensions...>;

template <typename T, unsigned... Dimensions>
using __EncodedBaseArray =
    GenericEncodedArray<T, bool, std::default_delete<bool[]>, void, void, void,
                          ::CleartextCopy, ::CleartextEncode, ::CleartextEncode,
                          ::CleartextDecode, Dimensions...>;


$2
#endif//$0)";

// Bool struct template
//  0: Struct name
//  1: Fully-qualified struct-type name
//  2: special insert for constructor and Decode method to string for fixed
//     arrays
//  3: special insert for constructor and Decode method to string for dynamic
//     arrays
constexpr const char kBoolStructTemplate[] = R"(
class Encoded$0Ref : public __EncodedBaseRef<$0> {
 public:
  using __EncodedBaseRef<$0>::__EncodedBaseRef;

  Encoded$0Ref(const __EncodedBaseRef<$0>& rhs)
      : Encoded$0Ref(const_cast<bool*>(rhs.get().data()), rhs.length()) {}

  void Encode(const $1& value) { SetEncrypted(value, nullptr); }

  $1 Decode() const { return Decrypt(nullptr); }

  using __EncodedBaseRef<$0>::get;

 private:
  using __EncodedBaseRef<$0>::BorrowedDecrypt;
  using __EncodedBaseRef<$0>::BorrowedSetEncrypted;
  using __EncodedBaseRef<$0>::BorrowedSetUnencrypted;
  using __EncodedBaseRef<$0>::Decrypt;
  using __EncodedBaseRef<$0>::SetEncrypted;
  using __EncodedBaseRef<$0>::SetUnencrypted;
};

class Encoded$0 : public __EncodedBase<$0> {
 public:
  Encoded$0()
      : __EncodedBase<$0>(new bool[Encoded$0::element_bit_width()], 1,
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

  using __EncodedBase<$0>::get;

 private:
  using __EncodedBase<$0>::BorrowedDecrypt;
  using __EncodedBase<$0>::BorrowedSetEncrypted;
  using __EncodedBase<$0>::BorrowedSetUnencrypted;
  using __EncodedBase<$0>::Decrypt;
  using __EncodedBase<$0>::SetEncrypted;
  using __EncodedBase<$0>::SetUnencrypted;
};

template <unsigned... Dimensions>
class Encoded$0Array;

template <>
class Encoded$0Array<> : public __EncodedBaseArray<$0> {
 public:
  Encoded$0Array(size_t length)
      : __EncodedBaseArray<$0>(new bool[length * Encoded$0::element_bit_width()],
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

  using __EncodedBaseArray<$0>::get;

 private:
  using __EncodedBaseArray<$0>::Decrypt;
  using __EncodedBaseArray<$0>::SetUnencrypted;
  using __EncodedBaseArray<$0>::SetEncrypted;
  using __EncodedBaseArray<$0>::BorrowedSetUnencrypted;
  using __EncodedBaseArray<$0>::BorrowedSetEncrypted;
  using __EncodedBaseArray<$0>::BorrowedDecrypt;
};

template <unsigned D1>
class Encoded$0Array<D1> : public __EncodedBaseArray<$0, D1> {
 public:
  Encoded$0Array()
      : __EncodedBaseArray<$0, D1>(
            new bool[__EncodedBaseArray<$0, D1>::element_bit_width() * D1], D1,
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

  void Encode(std::add_const_t<typename __EncodedBaseArray<$0, D1>::ArrayT> value) {
    SetEncrypted(value, nullptr, 0);
  }

  void Decode(typename __EncodedBaseArray<$0, D1>::ArrayT value) const {
    Decrypt(value, nullptr);
  }

  absl::FixedArray<$1> Decode() const { return Decrypt(nullptr); }

  using __EncodedBaseArray<$0, D1>::operator[];

  using __EncodedBaseArray<$0, D1>::get;

 private:
  using __EncodedBaseArray<$0, D1>::Decrypt;
  using __EncodedBaseArray<$0, D1>::SetUnencrypted;
  using __EncodedBaseArray<$0, D1>::SetEncrypted;
  using __EncodedBaseArray<$0, D1>::BorrowedSetUnencrypted;
  using __EncodedBaseArray<$0, D1>::BorrowedSetEncrypted;
  using __EncodedBaseArray<$0, D1>::BorrowedDecrypt;
};

template <unsigned... Dimensions>
class Encoded$0Array : public __EncodedBaseArray<$0, Dimensions...> {
 public:
  Encoded$0Array()
      : __EncodedBaseArray<$0, Dimensions...>(
            new bool[__EncodedBaseArray<$0, Dimensions...>::element_bit_width() *
                     __EncodedBaseArray<$0, Dimensions...>::VOLUME],
            __EncodedBaseArray<$0, Dimensions...>::VOLUME,
            std::default_delete<bool[]>()) {}

  // No initializer_list constructor for multi-dimensional arrays.

  void Encode(std::add_const_t<typename __EncodedBaseArray<$0, Dimensions...>::ArrayT> value) {
    SetEncrypted(value, nullptr, 0);
  }

  void Decode(typename __EncodedBaseArray<$0, Dimensions...>::ArrayT value) const {
    Decrypt(value, nullptr);
  }

  using __EncodedBaseArray<$0, Dimensions...>::operator[];

  using __EncodedBaseArray<$0, Dimensions...>::get;

 private:
  using __EncodedBaseArray<$0, Dimensions...>::Decrypt;
  using __EncodedBaseArray<$0, Dimensions...>::SetUnencrypted;
  using __EncodedBaseArray<$0, Dimensions...>::SetEncrypted;
  using __EncodedBaseArray<$0, Dimensions...>::BorrowedSetUnencrypted;
  using __EncodedBaseArray<$0, Dimensions...>::BorrowedSetEncrypted;
  using __EncodedBaseArray<$0, Dimensions...>::BorrowedDecrypt;
};

template <unsigned... Dimensions>
class Encoded$0ArrayRef;

template <>
class Encoded$0ArrayRef<> : public __EncodedBaseArrayRef<$0> {
 public:
  using __EncodedBaseArrayRef<$0>::__EncodedBaseArrayRef;
  Encoded$0ArrayRef(const __EncodedBaseArrayRef<$0>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()), rhs.length()) {}
  Encoded$0ArrayRef(const __EncodedBaseArray<$0>& rhs)
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

 private:
  using __EncodedBaseArrayRef<$0>::Decrypt;
  using __EncodedBaseArrayRef<$0>::SetUnencrypted;
  using __EncodedBaseArrayRef<$0>::SetEncrypted;
  using __EncodedBaseArrayRef<$0>::BorrowedSetUnencrypted;
  using __EncodedBaseArrayRef<$0>::BorrowedSetEncrypted;
  using __EncodedBaseArrayRef<$0>::BorrowedDecrypt;
};

template <unsigned D1>
class Encoded$0ArrayRef<D1> : public __EncodedBaseArrayRef<$0, D1> {
 public:
  using __EncodedBaseArrayRef<$0, D1>::__EncodedBaseArrayRef;

  Encoded$0ArrayRef(const __EncodedBaseArrayRef<$0, D1>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()), rhs.length()) {}
  Encoded$0ArrayRef(const __EncodedBaseArray<$0, D1>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()), rhs.length()) {}
  Encoded$0ArrayRef(const __EncodedBaseArray<$0>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()), rhs.length()) {
    XLS_CHECK_EQ(rhs.length(), D1);
  }

  void Encode(std::add_const_t<typename __EncodedBaseArrayRef<$0, D1>::ArrayT> value) {
    SetEncrypted(value, nullptr);
  }

  void Decode(typename __EncodedBaseArrayRef<$0, D1>::ArrayT value) const {
    Decrypt(value, nullptr);
  }

  absl::FixedArray<$1> Decode() const { return Decrypt(nullptr); }

 private:
  using __EncodedBaseArrayRef<$0, D1>::Decrypt;
  using __EncodedBaseArrayRef<$0, D1>::SetUnencrypted;
  using __EncodedBaseArrayRef<$0, D1>::SetEncrypted;
  using __EncodedBaseArrayRef<$0, D1>::BorrowedSetUnencrypted;
  using __EncodedBaseArrayRef<$0, D1>::BorrowedSetEncrypted;
  using __EncodedBaseArrayRef<$0, D1>::BorrowedDecrypt;
};

template <unsigned... Dimensions>
class Encoded$0ArrayRef : public __EncodedBaseArrayRef<$0, Dimensions...> {
 public:
  using __EncodedBaseArrayRef<$0, Dimensions...>::__EncodedBaseArrayRef;
  // Use the VOLUME constant instead of length().  The former gives the total
  // volume of the underlying array (accounting for all dimensions), while the
  // former gives the length of the first level of the array (e.g.,
  // Type[4][3][2] will have VOLUME = 24 and length() == 4.
  Encoded$0ArrayRef(const __EncodedBaseArrayRef<$0, Dimensions...>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()),
                          __EncodedBaseArrayRef<$0, Dimensions...>::VOLUME) {}
  Encoded$0ArrayRef(const __EncodedBaseArray<$0, Dimensions...>& rhs)
      : Encoded$0ArrayRef(const_cast<bool*>(rhs.get().data()),
                          __EncodedBaseArray<$0, Dimensions...>::VOLUME) {}

  void Encode(
      const typename __EncodedBaseArrayRef<$0, Dimensions...>::ArrayT value) {
    SetEncrypted(value, nullptr);
  }

  void Decode(
      typename __EncodedBaseArrayRef<$0, Dimensions...>::ArrayT value) const {
    Decrypt(value, nullptr);
  }

  using __EncodedBaseArrayRef<$0, Dimensions...>::get;

 private:
  using __EncodedBaseArrayRef<$0, Dimensions...>::Decrypt;
  using __EncodedBaseArrayRef<$0, Dimensions...>::SetUnencrypted;
  using __EncodedBaseArrayRef<$0, Dimensions...>::SetEncrypted;
  using __EncodedBaseArrayRef<$0, Dimensions...>::BorrowedSetUnencrypted;
  using __EncodedBaseArrayRef<$0, Dimensions...>::BorrowedSetEncrypted;
  using __EncodedBaseArrayRef<$0, Dimensions...>::BorrowedDecrypt;
};

template <>
class EncodedArray<$1> : public Encoded$0Array<> {
 public:
  using Encoded$0Array<>::Encoded$0Array;
  EncodedArray<$1>(std::initializer_list<$1> list) :
    Encoded$0Array(list) {}

$2
};

template <unsigned D1>
class EncodedArray<$1, D1> : public Encoded$0Array<D1> {
 public:
  using Encoded$0Array<D1>::Encoded$0Array;
  EncodedArray<$1, D1>(std::initializer_list<$1> list) :
    Encoded$0Array<D1>(list) {}

$3
};

template <unsigned... Dimensions>
class EncodedArray<$1, Dimensions...> : public Encoded$0Array<Dimensions...> {};

template <>
class EncodedArrayRef<$1> : public Encoded$0ArrayRef<> {
 public:
  using Encoded$0ArrayRef<>::Encoded$0ArrayRef;
  EncodedArrayRef(const EncodedArray<$1>& rhs)
      : EncodedArrayRef<$1>(const_cast<bool*>(rhs.get().data()), rhs.length()) {
  }
};

template <unsigned D1, unsigned... Dimensions>
class EncodedArrayRef<$1, D1, Dimensions...>
    : public Encoded$0ArrayRef<D1, Dimensions...> {
 public:
  using Encoded$0ArrayRef<D1, Dimensions...>::Encoded$0ArrayRef;
  EncodedArrayRef(const EncodedArray<$1, D1, Dimensions...>& rhs)
      : EncodedArrayRef<$1, D1, Dimensions...>(const_cast<bool*>(rhs.get().data()),
                        Encoded$0ArrayRef<D1, Dimensions...>::VOLUME) {

    XLS_CHECK_EQ(rhs.length(), D1);
  }
};
)";

// $0 -- struct type ("PrimitiveChar")
// $1 -- this class' template parameters ("char" or "char, D1")
// $2 -- CharT ("char")
// $3 -- base class' template parameters ("" or "D1")
constexpr absl::string_view kCleartextDecodeFromStringTemplate = R"(
  EncodedArray<$1>(std::basic_string<$2>& val) :
    Encoded$0Array<$3>(val.length() + 1) {
    this->Encode(val.data(), val.length() + 1);
  }

  EncodedArray<$1>(const $2* val) :
    Encoded$0Array<$3>(strlen(val) + 1) {
    this->Encode(val, strlen(val) + 1);
  }

  std::basic_string<$2> Decode() {
    const absl::FixedArray<$2> v = Encoded$0Array<$3>::Decode();
    return std::basic_string<$2>(v.begin(), v.end());
  }
)";

// $0 -- prefix ("Tfhe", "OpenFhe")
// $1 -- struct type ("PrimitiveChar")
// $2 -- char type ("char")
// $3 -- template ("<>, <D1>")
// $4 -- scheme-specific private-key parameter declarations
// $5 -- scheme-specific private-key parameter names
constexpr absl::string_view kDecodeFromStringTemplate = R"(
  std::basic_string<$2> Decrypt($4) {
    const absl::FixedArray<$2> v = $0$1Array$3::Decrypt($5);
    return std::basic_string<$2>(v.begin(), v.end());
  }
)";

absl::StatusOr<std::string> ConvertStructsToEncodedBool(
    absl::string_view generic_header,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view output_path, const std::vector<std::string>& unwrap) {
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

    std::string fully_qualified_name = struct_name.fully_qualified_name();
    std::string special_decode_dyn;
    std::string special_decode_fixed;
    if (std::find(unwrap.begin(), unwrap.end(), struct_name.name()) !=
        unwrap.end()) {
      if (struct_type.fields_size() != 1) {
        return absl::InvalidArgumentError(
            absl::StrFormat("Cannot unwrap struct %s, it has %d elements.",
                            struct_name.name(), struct_type.fields_size()));
      }

      const StructField& field = struct_type.fields().Get(0);
      fully_qualified_name = GetTypeName(field.type()).value();
      if (fully_qualified_name == "char") {
        special_decode_dyn = absl::Substitute(
            kCleartextDecodeFromStringTemplate, struct_name.name(), "char",
            fully_qualified_name, "");
        special_decode_fixed = absl::Substitute(
            kCleartextDecodeFromStringTemplate, struct_name.name(), "char, D1",
            fully_qualified_name, "D1");
      }
    }

    std::string struct_text = absl::Substitute(
        kBoolStructTemplate, struct_name.name(), fully_qualified_name,
        special_decode_dyn, special_decode_fixed);
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

template <typename T>
using __TfheBaseRef = GenericEncodedRef<T,
    LweSample, LweSampleArrayDeleter, TFheGateBootstrappingSecretKeySet,
    TFheGateBootstrappingCloudKeySet, TFheGateBootstrappingParameterSet,
    ::TfheCopy, ::TfheUnencrypted, ::TfheEncrypt, ::TfheDecrypt>;

template <typename T>
using __TfheBase = GenericEncoded<T,
    LweSample, LweSampleArrayDeleter, TFheGateBootstrappingSecretKeySet,
    TFheGateBootstrappingCloudKeySet, TFheGateBootstrappingParameterSet,
    ::TfheCopy, ::TfheUnencrypted, ::TfheEncrypt, ::TfheDecrypt>;

template <typename T, unsigned... Dimensions>
using __TfheBaseArray = GenericEncodedArray<T,
    LweSample, LweSampleArrayDeleter, TFheGateBootstrappingSecretKeySet,
    TFheGateBootstrappingCloudKeySet, TFheGateBootstrappingParameterSet,
    ::TfheCopy, ::TfheUnencrypted, ::TfheEncrypt, ::TfheDecrypt, Dimensions...>;

template <typename T, unsigned... Dimensions>
using __TfheBaseArrayRef = GenericEncodedArrayRef<T,
    LweSample, LweSampleArrayDeleter, TFheGateBootstrappingSecretKeySet,
    TFheGateBootstrappingCloudKeySet, TFheGateBootstrappingParameterSet,
    ::TfheCopy, ::TfheUnencrypted, ::TfheEncrypt, ::TfheDecrypt, Dimensions...>;

$2
#endif//$0)";

// Tfhe struct template
//  0: Type name
//  1: Fully-qualified type name
constexpr const char kTfheStructTemplate[] = R"(
class Tfhe$0Ref : public __TfheBaseRef<$0> {
 public:
  using __TfheBaseRef<$0>::__TfheBaseRef;

  Tfhe$0Ref(const __TfheBaseRef<$0>& rhs)
      : Tfhe$0Ref(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {}
};

class Tfhe$0 : public __TfheBase<$0> {
 public:
  Tfhe$0(const TFheGateBootstrappingParameterSet* params)
      : __TfheBase<$0>(new_gate_bootstrapping_ciphertext_array(
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
class Tfhe$0ArrayRef;

template <unsigned... Dimensions>
class Tfhe$0Array;

template <>
class Tfhe$0Array<> : public __TfheBaseArray<$0> {
 public:
  Tfhe$0Array(size_t length, const TFheGateBootstrappingParameterSet* params)
      : __TfheBaseArray<$0>(
            new_gate_bootstrapping_ciphertext_array(
                Tfhe$0::element_bit_width() * length, params),
            length,
            LweSampleArrayDeleter(Tfhe$0::element_bit_width() * length)) {}
};

template <unsigned D1>
class Tfhe$0Array<D1> : public __TfheBaseArray<$0, D1> {
 public:
  Tfhe$0Array(const TFheGateBootstrappingParameterSet* params)
      : __TfheBaseArray<$0, D1>(
            new_gate_bootstrapping_ciphertext_array(
                Tfhe$0::element_bit_width() * D1, params),
            D1, LweSampleArrayDeleter(Tfhe$0::element_bit_width() * D1)) {}
};

template <unsigned... Dimensions>
class Tfhe$0Array : public __TfheBaseArray<$0, Dimensions...> {
 public:
  Tfhe$0Array(const TFheGateBootstrappingParameterSet* params)
      : __TfheBaseArray<$0, Dimensions...>(
            new_gate_bootstrapping_ciphertext_array(
                Tfhe$0::element_bit_width() *
                    __TfheBaseArray<$0, Dimensions...>::VOLUME,
                params),
            __TfheBaseArray<$0, Dimensions...>::VOLUME,
            LweSampleArrayDeleter(Tfhe$0::element_bit_width() *
                                  __TfheBaseArray<$0, Dimensions...>::VOLUME)) {}
};

template <unsigned... Dimensions>
class Tfhe$0ArrayRef;

template <unsigned... Dimensions>
class Tfhe$0Array;

template <>
class Tfhe$0ArrayRef<> : public __TfheBaseArrayRef<$0> {
 public:
  using __TfheBaseArrayRef<$0>::__TfheBaseArrayRef;
  Tfhe$0ArrayRef(const __TfheBaseArrayRef<$0>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {
  }
  Tfhe$0ArrayRef(const __TfheBaseArray<$0>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {
  }

  using __TfheBaseArrayRef<$0>::get;
};

template <unsigned D1>
class Tfhe$0ArrayRef<D1> : public __TfheBaseArrayRef<$0, D1> {
 public:
  using __TfheBaseArrayRef<$0, D1>::__TfheBaseArrayRef;
  Tfhe$0ArrayRef(const __TfheBaseArrayRef<$0, D1>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {
  }
  Tfhe$0ArrayRef(const __TfheBaseArray<$0, D1>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {
  }
  Tfhe$0ArrayRef(const __TfheBaseArray<$0>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()), rhs.length()) {
    XLS_CHECK_GE(rhs.length(), D1);
  }
  using __TfheBaseArrayRef<$0, D1>::get;
};

template <unsigned... Dimensions>
class Tfhe$0ArrayRef : public __TfheBaseArrayRef<$0, Dimensions...> {
 public:
  using __TfheBaseArrayRef<$0, Dimensions...>::__TfheBaseArrayRef;
  // Use the VOLUME constant instead of length().  The former gives the total
  // volume of the underlying array (accounting for all dimensions), while the
  // former gives the length of the first level of the array (e.g.,
  // Type[4][3][2] will have VOLUME = 24 and length() == 4.
  Tfhe$0ArrayRef(const __TfheBaseArrayRef<$0, Dimensions...>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()),
                       __TfheBaseArrayRef<$0, Dimensions...>::VOLUME) {}
  Tfhe$0ArrayRef(const __TfheBaseArray<$0, Dimensions...>& rhs)
      : Tfhe$0ArrayRef(const_cast<LweSample*>(rhs.get().data()),
                       __TfheBaseArray<$0, Dimensions...>::VOLUME) {}
  using __TfheBaseArrayRef<$0, Dimensions...>::get;
};

template <>
class TfheArrayRef<$1> : public Tfhe$0ArrayRef<> {
 public:
  using Tfhe$0ArrayRef<>::Tfhe$0ArrayRef;
  using Tfhe$0ArrayRef<>::get;
};

template <unsigned... Dimensions>
class TfheArrayRef<$1, Dimensions...> : public Tfhe$0ArrayRef<Dimensions...> {
 public:
  using Tfhe$0ArrayRef<Dimensions...>::Tfhe$0ArrayRef;
  TfheArrayRef(const TfheArray<$1, Dimensions...>& rhs)
      : TfheArrayRef<$1, Dimensions...>(
            const_cast<LweSample*>(rhs.get().data()),
            Tfhe$0ArrayRef<Dimensions...>::VOLUME) {}
  using Tfhe$0ArrayRef<Dimensions...>::get;
};

template <>
class TfheArray<$1> : public Tfhe$0Array<> {
 public:
  using Tfhe$0Array<>::Tfhe$0Array;
  using Tfhe$0Array<>::get;

  static TfheArray<$1> Unencrypted(
     absl::Span<const $1> plaintext,
     const TFheGateBootstrappingCloudKeySet* key) {
    TfheArray<$1> shared_value(plaintext.length(), key->params);
    shared_value.SetUnencrypted(plaintext.data(), plaintext.length(), key);
    return shared_value;
  }

  static TfheArray<$1> Encrypt(
      absl::Span<const $1> plaintext,
      const TFheGateBootstrappingSecretKeySet* key) {
    TfheArray<$1> private_value(plaintext.length(), key->params);
    private_value.SetEncrypted(plaintext.data(), plaintext.length(), key);
    return private_value;
  }
$2
};

template <unsigned D1>
class TfheArray<$1, D1> : public Tfhe$0Array<D1> {
 public:
  using Tfhe$0Array<D1>::Tfhe$0Array;
  using Tfhe$0Array<D1>::get;

  static TfheArray<$1, D1> Unencrypted(
     absl::Span<const $1> plaintext,
     const TFheGateBootstrappingCloudKeySet* key) {
    XLS_CHECK_EQ(plaintext.length(), D1);
    TfheArray<$1, D1> shared_value(key->params);
    shared_value.SetUnencrypted(plaintext.data(), key);
    return shared_value;
  }

  static TfheArray<$1, D1> Encrypt(
      absl::Span<const $1> plaintext,
      const TFheGateBootstrappingSecretKeySet* key) {
    XLS_CHECK_EQ(plaintext.length(), D1);
    TfheArray<$1, D1> private_value(key->params);
    private_value.SetEncrypted(plaintext.data(), key);
    return private_value;
  }
$3
};

template <unsigned D1, unsigned... Dimensions>
class TfheArray<$1, D1, Dimensions...> : public Tfhe$0Array<D1, Dimensions...> {
 public:
  using Tfhe$0Array<D1, Dimensions...>::Tfhe$0Array;
  using Tfhe$0Array<D1, Dimensions...>::get;

  static TfheArray<$1, D1, Dimensions...> Unencrypted(
     absl::Span<const $1> plaintext,
     const TFheGateBootstrappingCloudKeySet* key) {
    XLS_CHECK_EQ(plaintext.length(), D1);
    TfheArray<$1, D1> shared_value(key->params);
    shared_value.SetUnencrypted(plaintext.data(), key);
    return shared_value;
  }

  static TfheArray<$1, D1, Dimensions...> Encrypt(
      absl::Span<const $1> plaintext,
      const TFheGateBootstrappingSecretKeySet* key) {
    XLS_CHECK_EQ(plaintext.length(), D1);
    TfheArray<$1, D1> private_value(key->params);
    private_value.SetEncrypted(plaintext.data(), key);
    return private_value;
  }
};

)";

absl::StatusOr<std::string> ConvertStructsToEncodedTfhe(
    absl::string_view generic_header,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view output_path, const std::vector<std::string>& unwrap) {
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

    std::string fully_qualified_name = struct_name.fully_qualified_name();
    std::string special_decode_dyn;
    std::string special_decode_fixed;
    if (std::find(unwrap.begin(), unwrap.end(), struct_name.name()) !=
        unwrap.end()) {
      if (struct_type.fields_size() != 1) {
        return absl::InvalidArgumentError(
            absl::StrFormat("Cannot unwrap struct %s, it has %d elements.",
                            struct_name.name(), struct_type.fields_size()));
      }
      const StructField& field = struct_type.fields().Get(0);
      fully_qualified_name = GetTypeName(field.type()).value();
      if (fully_qualified_name == "char") {
        special_decode_dyn = absl::Substitute(
            kDecodeFromStringTemplate, "Tfhe", struct_name.name(),
            fully_qualified_name, "<>",
            "const TFheGateBootstrappingSecretKeySet* key", "key");
        special_decode_fixed = absl::Substitute(
            kDecodeFromStringTemplate, "Tfhe", struct_name.name(),
            fully_qualified_name, "<D1>",
            "const TFheGateBootstrappingSecretKeySet* key", "key");
      }
    }

    std::string struct_text = absl::Substitute(
        kTfheStructTemplate, struct_name.name(), fully_qualified_name,
        special_decode_dyn, special_decode_fixed);
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

template <typename T>
using __OpenFheBaseRef = GenericEncodedRef<T,
    lbcrypto::LWECiphertext, std::default_delete<lbcrypto::LWECiphertext[]>,
    OpenFhePrivateKeySet, lbcrypto::BinFHEContext, void, ::OpenFheCopy,
    ::OpenFheUnencrypted, ::OpenFheEncrypt, ::OpenFheDecrypt>;

template <typename T>
using __OpenFheBase = GenericEncoded<T,
    lbcrypto::LWECiphertext, std::default_delete<lbcrypto::LWECiphertext[]>,
    OpenFhePrivateKeySet, lbcrypto::BinFHEContext, void, ::OpenFheCopy,
    ::OpenFheUnencrypted, ::OpenFheEncrypt, ::OpenFheDecrypt>;

template <typename T, unsigned... Dimensions>
using __OpenFheBaseArray = GenericEncodedArray<T,
    lbcrypto::LWECiphertext, std::default_delete<lbcrypto::LWECiphertext[]>,
    OpenFhePrivateKeySet, lbcrypto::BinFHEContext, void, ::OpenFheCopy,
    ::OpenFheUnencrypted, ::OpenFheEncrypt, ::OpenFheDecrypt, Dimensions...>;

template <typename T, unsigned... Dimensions>
using __OpenFheBaseArrayRef = GenericEncodedArrayRef<T,
    lbcrypto::LWECiphertext, std::default_delete<lbcrypto::LWECiphertext[]>,
    OpenFhePrivateKeySet, lbcrypto::BinFHEContext, void, ::OpenFheCopy,
    ::OpenFheUnencrypted, ::OpenFheEncrypt, ::OpenFheDecrypt, Dimensions...>;

$2
#endif//$0)";

// OpenFhe struct template
//  0: Type name
//  1: Fully-qualified type name
constexpr const char kOpenFheStructTemplate[] = R"(
class OpenFhe$0Ref : public __OpenFheBaseRef<$0> {
 public:
  OpenFhe$0Ref(lbcrypto::LWECiphertext* data, size_t length,
                lbcrypto::BinFHEContext cc)
      : __OpenFheBaseRef<$0>(data, length), cc_(cc) {}
  OpenFhe$0Ref(const __OpenFheBaseRef<$0>& rhs, lbcrypto::BinFHEContext cc)
      : OpenFhe$0Ref(const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
                      rhs.length(), cc) {}
  OpenFhe$0Ref(const OpenFhe$0Ref& rhs)
      : OpenFhe$0Ref(const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
                      rhs.length(), rhs.cc_) {}

  void SetEncrypted(const $1& value, lbcrypto::LWEPrivateKey sk,
                    size_t elem = 0) {
    OpenFhePrivateKeySet key{cc_, sk};
    __OpenFheBaseRef<$0>::SetEncrypted(value, &key, elem);
  }

  $1 Decrypt(lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    return __OpenFheBaseRef<$0>::Decrypt(&key, elem);
  }

 private:
  using __OpenFheBaseRef<$0>::__OpenFheBaseRef;
  lbcrypto::BinFHEContext cc_;
};

class OpenFhe$0 : public __OpenFheBase<$0> {
 public:
  OpenFhe$0(lbcrypto::BinFHEContext cc)
      : __OpenFheBase<$0>(
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
    __OpenFheBase<$0>::SetEncrypted(value, &key, elem);
  }

  $1 Decrypt(lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    return __OpenFheBase<$0>::Decrypt(&key, elem);
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

template <>
class OpenFhe$0Array<> : public __OpenFheBaseArray<$0> {
 public:
  OpenFhe$0Array(size_t length, lbcrypto::BinFHEContext cc)
      : __OpenFheBaseArray<$0>(
            new lbcrypto::LWECiphertext[OpenFhe$0::element_bit_width() *
                                        length],
            length, std::default_delete<lbcrypto::LWECiphertext[]>()),
        cc_(cc) {
    // Initialize the LWECiphertexts.
    for (int i = 0; i < length; i++) {
      __OpenFheBase<$0>::SetUnencrypted({}, &cc_, i);
    }
  }

  void SetEncrypted(const $1* value, size_t length,
                    lbcrypto::LWEPrivateKey sk) {
    OpenFhePrivateKeySet key{cc_, sk};
    __OpenFheBaseArray<$0>::SetEncrypted(value, length, &key);
  }

  void SetEncrypted(absl::Span<const $1> value, lbcrypto::LWEPrivateKey sk) {
    SetEncrypted(value.data(), value.size(), sk);
  }

  void Decrypt($1* value, size_t length, lbcrypto::LWEPrivateKey sk) const {
    OpenFhePrivateKeySet key{cc_, sk};
    __OpenFheBaseArray<$0>::Decrypt(value, length, &key);
  }

  absl::FixedArray<$1> Decrypt(lbcrypto::LWEPrivateKey sk) const {
    OpenFhePrivateKeySet key{cc_, sk};
    return __OpenFheBaseArray<$0>::Decrypt(&key);
  }

  OpenFhe$0Ref operator[](size_t pos) {
    auto ref = this->__OpenFheBaseArray<$0>::operator[](pos);
    return OpenFhe$0Ref(ref, cc_);
  }

 private:
  friend OpenFhe$0ArrayRef<>;
  template <unsigned...> friend class OpenFhe$0ArrayRef;
  lbcrypto::BinFHEContext cc_;
};

template <unsigned D1>
class OpenFhe$0Array<D1> : public __OpenFheBaseArray<$0, D1> {
 public:
  OpenFhe$0Array(lbcrypto::BinFHEContext cc)
      : __OpenFheBaseArray<$0, D1>(
            new lbcrypto::LWECiphertext[OpenFhe$0::element_bit_width() * D1],
            D1, std::default_delete<lbcrypto::LWECiphertext[]>()),
        cc_(cc) {
    // Initialize the LWECiphertexts.
    for (int i = 0; i < D1; i++) {
      __OpenFheBase<$0>::SetUnencrypted({}, &cc_, i);
    }
  }

  void SetEncrypted(const typename __OpenFheBaseArray<$0, D1>::ArrayT value,
                    lbcrypto::LWEPrivateKey sk, size_t elem = 0) {
    OpenFhePrivateKeySet key{cc_, sk};
    __OpenFheBaseArray<$0, D1>::SetEncrypted(value, &key, elem);
  }

  void SetEncrypted(absl::Span<const $1> value, lbcrypto::LWEPrivateKey sk) {
    XLS_CHECK_EQ(value.size(), D1);
    SetEncrypted(value.data(), value.size(), sk);
  }

  void Decrypt(typename __OpenFheBaseArray<$0, D1>::ArrayT value,
               lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    __OpenFheBaseArray<$0, D1>::Decrypt(value, &key, elem);
  }

  absl::FixedArray<$1> Decrypt(lbcrypto::LWEPrivateKey sk) const {
    OpenFhePrivateKeySet key{cc_, sk};
    return __OpenFheBaseArray<$0, D1>::Decrypt(&key);
  }

  OpenFhe$0Ref operator[](size_t pos) {
    auto ref = this->__OpenFheBaseArray<$0, D1>::operator[](pos);
    return OpenFhe$0Ref(ref, cc_);
  }

 private:
  friend OpenFhe$0ArrayRef<D1>;
  lbcrypto::BinFHEContext cc_;
};

template <unsigned D1, unsigned... Dimensions>
class OpenFhe$0Array<D1, Dimensions...>
    : public __OpenFheBaseArray<$0, D1, Dimensions...> {
 public:
  OpenFhe$0Array(lbcrypto::BinFHEContext cc)
      : __OpenFheBaseArray<$0, D1, Dimensions...>(
            new lbcrypto::LWECiphertext
                [OpenFhe$0::element_bit_width() *
                 __OpenFheBaseArray<$0, D1, Dimensions...>::VOLUME],
            __OpenFheBaseArray<$0, D1, Dimensions...>::VOLUME,
            std::default_delete<lbcrypto::LWECiphertext[]>()),
        cc_(cc) {
    // Initialize the LWECiphertexts.
    for (int i = 0; i < __OpenFheBaseArray<$0, D1, Dimensions...>::VOLUME; i++) {
      __OpenFheBase<$0>::SetUnencrypted({}, &cc_, i);
    }
  }

  void SetEncrypted(
      const typename __OpenFheBaseArray<$0, D1, Dimensions...>::ArrayT value,
      lbcrypto::LWEPrivateKey sk, size_t elem = 0) {
    OpenFhePrivateKeySet key{cc_, sk};
    __OpenFheBaseArray<$0, D1, Dimensions...>::SetEncrypted(value, &key, elem);
  }

  void Decrypt(typename __OpenFheBaseArray<$0, D1, Dimensions...>::ArrayT value,
               lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    __OpenFheBaseArray<$0, D1, Dimensions...>::Decrypt(value, &key, elem);
  }

  OpenFhe$0ArrayRef<Dimensions...> operator[](size_t pos) {
    auto ref = this->__OpenFheBaseArray<$0, D1, Dimensions...>::operator[](pos);
    return OpenFhe$0ArrayRef<Dimensions...>(ref, cc_);
  }

 private:
  friend OpenFhe$0ArrayRef<D1, Dimensions...>;
  lbcrypto::BinFHEContext cc_;
};

template <>
class OpenFhe$0ArrayRef<> : public __OpenFheBaseArrayRef<$0> {
 public:
  OpenFhe$0ArrayRef(const OpenFhe$0ArrayRef<>& rhs)
      : __OpenFheBaseArrayRef<$0>(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            rhs.length()) {
    cc_ = rhs.cc_;
  }
  OpenFhe$0ArrayRef(const OpenFhe$0Array<>& rhs)
      : __OpenFheBaseArrayRef<$0>(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            rhs.length()) {
    cc_ = rhs.cc_;
  }

  void Decrypt($1* value, size_t length,
               lbcrypto::LWEPrivateKey sk) const {
    OpenFhePrivateKeySet key{cc_, sk};
    __OpenFheBaseArrayRef<$0>::Decrypt(value, length, &key);
  }

  absl::FixedArray<$1> Decrypt(lbcrypto::LWEPrivateKey sk) const {
    OpenFhePrivateKeySet key{cc_, sk};
    return __OpenFheBaseArrayRef<$0>::Decrypt(&key);
  }

  OpenFhe$0Ref operator[](size_t pos) {
    auto ref = this->__OpenFheBaseArrayRef<$0>::operator[](pos);
    return OpenFhe$0Ref(ref, cc_);
  }

 private:
  lbcrypto::BinFHEContext cc_;
};

template <unsigned D1>
class OpenFhe$0ArrayRef<D1> : public __OpenFheBaseArrayRef<$0, D1> {
 public:
  using __OpenFheBaseArrayRef<$0, D1>::__OpenFheBaseArrayRef;
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
  OpenFhe$0ArrayRef(const __OpenFheBaseArrayRef<$0, D1>& rhs,
                     lbcrypto::BinFHEContext cc)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            __OpenFheBaseArrayRef<$0, D1>::VOLUME) {
    cc_ = cc;
  }
  OpenFhe$0ArrayRef(absl::Span<const lbcrypto::LWECiphertext> data,
                     lbcrypto::BinFHEContext cc)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(data.data()),
            __OpenFheBaseArrayRef<$0, D1>::VOLUME) {
    XLS_CHECK_EQ(data.length(), this->bit_width());
    cc_ = cc;
  }

  OpenFhe$0Ref operator[](size_t pos) {
    auto ref = this->__OpenFheBaseArrayRef<$0, D1>::operator[](pos);
    return OpenFhe$0Ref(ref, cc_);
  }

  void SetEncrypted(const typename __OpenFheBaseArrayRef<$0, D1>::ArrayT value,
                    lbcrypto::LWEPrivateKey sk, size_t elem = 0) {
    OpenFhePrivateKeySet key{cc_, sk};
    this->__OpenFheBaseArrayRef<$0, D1>::SetEncrypted(value, &key, elem);
  }

  void Decrypt(typename __OpenFheBaseArrayRef<$0, D1>::ArrayT value,
               lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    this->__OpenFheBaseArrayRef<$0, D1>::Decrypt(value, &key, elem);
  }

  absl::FixedArray<$1> Decrypt(lbcrypto::LWEPrivateKey sk) const {
    OpenFhePrivateKeySet key{cc_, sk};
    return __OpenFheBaseArrayRef<$0, D1>::Decrypt(&key);
  }

  lbcrypto::BinFHEContext cc() { return cc_; }
 private:
  lbcrypto::BinFHEContext cc_;
};

template <unsigned D1, unsigned... Dimensions>
class OpenFhe$0ArrayRef<D1, Dimensions...>
    : public __OpenFheBaseArrayRef<$0, D1, Dimensions...> {
 public:
  using __OpenFheBaseArrayRef<$0, D1, Dimensions...>::__OpenFheBaseArrayRef;
  // Use the VOLUME constant instead of length().  The former gives the total
  // volume of the underlying array (accounting for all dimensions), while the
  // former gives the length of the first level of the array (e.g.,
  // Type[4][3][2] will have VOLUME = 24 and length() == 4.
  OpenFhe$0ArrayRef(const OpenFhe$0ArrayRef<D1, Dimensions...>& rhs)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            __OpenFheBaseArrayRef<$0, D1, Dimensions...>::VOLUME) {
    cc_ = rhs.cc_;
  }
  OpenFhe$0ArrayRef(const OpenFhe$0Array<D1, Dimensions...>& rhs)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            __OpenFheBaseArrayRef<$0, D1, Dimensions...>::VOLUME) {
    cc_ = rhs.cc_;
  }
  OpenFhe$0ArrayRef(const __OpenFheBaseArrayRef<$0, D1, Dimensions...>& rhs,
                     lbcrypto::BinFHEContext cc)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(rhs.get().data()),
            __OpenFheBaseArrayRef<$0, D1, Dimensions...>::VOLUME) {
    cc_ = cc;
  }
  OpenFhe$0ArrayRef(absl::Span<const lbcrypto::LWECiphertext> data,
                     lbcrypto::BinFHEContext cc)
      : OpenFhe$0ArrayRef(
            const_cast<lbcrypto::LWECiphertext*>(data.data()),
            __OpenFheBaseArrayRef<$0, D1, Dimensions...>::VOLUME) {
    XLS_CHECK_EQ(data.length(), this->bit_width());
    cc_ = cc;
  }

  OpenFhe$0ArrayRef<Dimensions...> operator[](size_t pos) {
    auto ref = this->__OpenFheBaseArrayRef<$0, D1, Dimensions...>::operator[](pos);
    return OpenFhe$0ArrayRef<Dimensions...>(ref, cc_);
  }

  void SetEncrypted(
      const typename __OpenFheBaseArrayRef<$0, D1, Dimensions...>::ArrayT value,
      lbcrypto::LWEPrivateKey sk, size_t elem = 0) {
    OpenFhePrivateKeySet key{cc_, sk};
    this->__OpenFheBaseArrayRef<$0, D1, Dimensions...>::SetEncrypted(value, &key,
                                                                  elem);
  }

  void Decrypt(typename __OpenFheBaseArrayRef<$0, D1, Dimensions...>::ArrayT value,
               lbcrypto::LWEPrivateKey sk, size_t elem = 0) const {
    OpenFhePrivateKeySet key{cc_, sk};
    this->__OpenFheBaseArrayRef<$0, D1, Dimensions...>::Decrypt(value, &key, elem);
  }

  lbcrypto::BinFHEContext cc() { return cc_; }
 private:
  lbcrypto::BinFHEContext cc_;
};

template <>
class OpenFheArray<$1> : public OpenFhe$0Array<> {
 public:
  using OpenFhe$0Array<>::OpenFhe$0Array;

  static OpenFheArray<$1> Encrypt(
      absl::Span<const $1> plaintext, lbcrypto::BinFHEContext cc,
      lbcrypto::LWEPrivateKey sk) {
    OpenFheArray<$1> private_value(plaintext.length(), cc);
    private_value.SetEncrypted(plaintext.data(), plaintext.length(), sk);
    return private_value;
  }
$2
};

template <unsigned D1>
class OpenFheArray<$1, D1> : public OpenFhe$0Array<D1> {
 public:
  using OpenFhe$0Array<D1>::OpenFhe$0Array;

  static OpenFheArray<$1> Encrypt(
      absl::Span<const $1> plaintext, lbcrypto::BinFHEContext cc,
      lbcrypto::LWEPrivateKey sk) {
    OpenFheArray<$1, D1> private_value(plaintext.length(), cc);
    private_value.SetEncrypted(plaintext.data(), plaintext.length(), sk);
    return private_value;
  }
$2
};

template <unsigned D1, unsigned... Dimensions>
class OpenFheArray<$1, D1, Dimensions...> : public OpenFhe$0Array<D1, Dimensions...> {
 public:
  using OpenFhe$0Array<D1, Dimensions...>::OpenFhe$0Array;

  OpenFheArrayRef<$1, Dimensions...> operator[](size_t pos) {
    auto ref = this->OpenFhe$0Array<D1, Dimensions...>::operator[](pos);
    return OpenFheArrayRef<$1, Dimensions...>(ref.get(), ref.cc());
  }
};

template <>
class OpenFheArrayRef<$1> : public OpenFhe$0ArrayRef<> {
 public:
  using OpenFhe$0ArrayRef<>::OpenFhe$0ArrayRef;
};

template <unsigned D1>
class OpenFheArrayRef<$1, D1>
    : public OpenFhe$0ArrayRef<D1> {
 public:
  using OpenFhe$0ArrayRef<D1>::OpenFhe$0ArrayRef;
};

template <unsigned D1, unsigned... Dimensions>
class OpenFheArrayRef<$1, D1, Dimensions...>
    : public OpenFhe$0ArrayRef<D1, Dimensions...> {
 public:
  using OpenFhe$0ArrayRef<D1, Dimensions...>::OpenFhe$0ArrayRef;

  OpenFheArrayRef<$1, Dimensions...> operator[](size_t pos) {
    auto ref = this->OpenFhe$0ArrayRef<D1, Dimensions...>::operator[](pos);
    return OpenFheArrayRef<$1, Dimensions...>(ref.get(), ref.cc());
  }
};
)";

absl::StatusOr<std::string> ConvertStructsToEncodedOpenFhe(
    absl::string_view generic_header,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view output_path, const std::vector<std::string>& unwrap) {
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

    std::string fully_qualified_name = struct_name.fully_qualified_name();
    std::string special_decode_fixed;
    std::string special_decode_dyn;
    if (std::find(unwrap.begin(), unwrap.end(), struct_name.name()) !=
        unwrap.end()) {
      if (struct_type.fields_size() != 1) {
        return absl::InvalidArgumentError(
            absl::StrFormat("Cannot unwrap struct %s, it has %d elements.",
                            struct_name.name(), struct_type.fields_size()));
      }
      const StructField& field = struct_type.fields().Get(0);
      fully_qualified_name = GetTypeName(field.type()).value();
      if (fully_qualified_name == "char") {
        special_decode_dyn = absl::Substitute(
            kDecodeFromStringTemplate, "OpenFhe", struct_name.name(),
            fully_qualified_name, "<>", "lbcrypto::LWEPrivateKey sk", "sk");
        special_decode_fixed = absl::Substitute(
            kDecodeFromStringTemplate, "OpenFhe", struct_name.name(),
            fully_qualified_name, "<D1>", "lbcrypto::LWEPrivateKey sk", "sk");
      }
    }

    std::string struct_text = absl::Substitute(
        kOpenFheStructTemplate, struct_name.name(), fully_qualified_name,
        special_decode_dyn, special_decode_fixed);
    generated.push_back(struct_text);
  }

  return absl::Substitute(kOpenFheFileTemplate, header_guard, generic_header,
                          absl::StrJoin(generated, "\n\n"));
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
