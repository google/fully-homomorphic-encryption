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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_STRUCT_TRANSPILER_CONVERT_STRUCT_TO_ENCODED_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_STRUCT_TRANSPILER_CONVERT_STRUCT_TO_ENCODED_H_

#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

absl::StatusOr<std::string> ConvertStructsToEncodedTemplate(
    const xlscc_metadata::MetadataOutput& metadata,
    const std::vector<std::string>& original_headers,
    absl::string_view output_path, const std::vector<std::string>& unwrap);

absl::StatusOr<std::string> ConvertStructsToEncodedBool(
    absl::string_view generic_header,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view output_path, const std::vector<std::string>& unwrap);

absl::StatusOr<std::string> ConvertStructsToEncodedTfhe(
    absl::string_view generic_header,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view output_path, const std::vector<std::string>& unwrap);

absl::StatusOr<std::string> ConvertStructsToEncodedOpenFhe(
    absl::string_view generic_header,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view output_path, const std::vector<std::string>& unwrap);

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_STRUCT_TRANSPILER_CONVERT_STRUCT_TO_ENCODED_H_
