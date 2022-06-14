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

// Helper functions for Boolean YoSys Transpiler.

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_TRANSPILER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_TRANSPILER_H_

#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "transpiler/pipeline_enums.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

// Converts a netlist in Verilog into a C++ function that invokes the
// XLS netlist interpreter.
class YosysTranspiler {
 public:
  static absl::StatusOr<std::string> Translate(
      const xlscc_metadata::MetadataOutput& metadata,
      const absl::string_view cell_library_text,
      const absl::string_view netlist_text, Encryption encryption);

  static absl::StatusOr<std::string> TranslateHeader(
      const xlscc_metadata::MetadataOutput& metadata,
      absl::string_view header_path, Encryption encryption,
      const absl::string_view
          encryption_specific_transpiled_structs_header_path,
      const std::vector<std::string>& unwrap);

  static absl::StatusOr<std::string> FunctionSignature(
      const xlscc_metadata::MetadataOutput& metadata, Encryption encryption);

 private:
  static absl::StatusOr<std::string> PathToHeaderGuard(
      absl::string_view header_path, Encryption encryption);
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_TRANSPILER_H_
