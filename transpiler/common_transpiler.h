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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_COMMON_TRANSPILER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_COMMON_TRANSPILER_H_

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/optional.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

// Simple holder for a type and its total bit width.
struct TypeData {
  xlscc_metadata::StructType type;
  size_t bit_width;
};

using IdToType = absl::flat_hash_map<int64_t, TypeData>;

absl::optional<std::string> GetTypeName(
    const xlscc_metadata::InstanceType& type);
absl::optional<std::string> GetTypeName(const xlscc_metadata::Type& type);

absl::optional<std::string> TypedOverload(
    const xlscc_metadata::MetadataOutput& metadata,
    const absl::string_view prefix, const std::string default_type,
    absl::optional<absl::string_view> key_param_type,
    absl::string_view key_param_name, const std::vector<std::string>& unwrap);

std::string FunctionSignature(const xlscc_metadata::MetadataOutput& metadata,
                              const absl::string_view element_type,
                              absl::optional<absl::string_view> key_param_type,
                              absl::string_view key_param_name = "bk");

std::string PathToHeaderGuard(std::string_view default_value,
                              std::string_view header_path);

size_t GetBitWidth(const IdToType& id_to_type,
                   const xlscc_metadata::Type& type);
size_t GetStructWidth(const IdToType& id_to_type,
                      const xlscc_metadata::StructType& struct_type);

// Gets the order in which we should process any struct definitions held in the
// given MetadataOutput.
// Since we can't assume any given ordering of output structs, we need to
// toposort.
std::vector<int64_t> GetTypeReferenceOrder(
    const xlscc_metadata::MetadataOutput& metadata);

// Loads an IdToType with the necessary data (type bit width, really).
IdToType PopulateTypeData(const xlscc_metadata::MetadataOutput& metadata,
                          const std::vector<int64_t>& processing_order);

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_COMMON_TRANSPILER_H_
