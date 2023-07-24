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

// Helper functions to convert TF2FHE metadata to xlscc metadata.

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_METADATA_UTILS_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_METADATA_UTILS_H_

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/netlist/netlist.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

// CreateMetadataFromJson generates an xlscc_metadata from a JSON string payload
// that contains HEIR metadata.
// See https://google.github.io/heir/docs/pipelines/#--emit-metadata
absl::StatusOr<xlscc_metadata::MetadataOutput> CreateMetadataFromHeirJson(
    absl::string_view metadata_str,
    const std::unique_ptr<xls::netlist::rtl::AbstractModule<bool>>& module);

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_METADATA_UTILS_H_
