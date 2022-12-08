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

// Helper functions for FHE IR Transpiler.

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_INTERPRETED_TFHE_TRANSPILER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_INTERPRETED_TFHE_TRANSPILER_H_

#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/public/ir.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

// Converts booleanified XLS functions into a C++ function that invokes a
// TFHE-based interpreter for each gate.
class InterpretedTfheTranspiler {
 public:
  // Takes as input an XLS Function node and expected output and returns an FHE
  // C++ method that uses the gate ops from TFHE library.
  static absl::StatusOr<std::string> Translate(
      const xls::Function* function,
      const xlscc_metadata::MetadataOutput& metadata);

  // Takes as input an XLS Function node and returns an FHE
  // C++ header file.
  static absl::StatusOr<std::string> TranslateHeader(
      const xls::Function* function,
      const xlscc_metadata::MetadataOutput& metadata,
      absl::string_view header_path,
      const absl::string_view
          encryption_specific_transpiled_structs_header_path,
      bool skip_scheme_data_deps, const std::vector<std::string>& unwrap);

  static absl::StatusOr<std::string> FunctionSignature(
      const xls::Function* function,
      const xlscc_metadata::MetadataOutput& metadata);

 private:
  static absl::StatusOr<std::string> PathToHeaderGuard(
      absl::string_view header_path);
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_INTERPRETED_TFHE_TRANSPILER_H_
