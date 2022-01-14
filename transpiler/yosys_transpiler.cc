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

#include "transpiler/yosys_transpiler.h"

#include <string>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/text_format.h"
#include "xls/common/logging/logging.h"
#include "xls/common/status/status_macros.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

absl::StatusOr<std::string> YosysTranspiler::Translate(
    const xlscc_metadata::MetadataOutput& metadata,
    const absl::string_view cell_library_text,
    const absl::string_view netlist_text,
    const absl::string_view transpiler_type) {
  static constexpr absl::string_view kSourceTemplate =
      R"source(#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "xls/common/status/status_macros.h"

$6

namespace {

static constexpr char kNetlistPackage[] = R"ir($0)ir";

static constexpr char kFunctionMetadata[] = R"pb(
$1
)pb";

static constexpr char kCellDefinitions[] = R"cd(
$2
)cd";

static fully_homomorphic_encryption::transpiler::Yosys$7Runner runner(
                            kCellDefinitions,
                            kNetlistPackage,
                            kFunctionMetadata);

}  // namespace

$3 {
  return runner.Run($4, {$5}$8);
})source";

  XLS_ASSIGN_OR_RETURN(const std::string signature,
                       FunctionSignature(metadata, transpiler_type));
  std::string template_type =
      transpiler_type == "yosys_plaintext" ? "bool" : "LweSample";
  std::string return_param =
      absl::Substitute("absl::Span<$0>()", template_type);

  if (!metadata.top_func_proto().return_type().has_as_void()) {
    return_param = "result";
  }
  std::vector<std::string> param_entries;
  for (auto& param : metadata.top_func_proto().params()) {
    param_entries.push_back(param.name());
  }

  // Serialize the metadata, removing the trailing null.
  std::string metadata_text;
  XLS_CHECK(
      google::protobuf::TextFormat::PrintToString(metadata, &metadata_text));

  return absl::Substitute(
      kSourceTemplate, netlist_text, metadata_text, cell_library_text,
      signature, return_param, absl::StrJoin(param_entries, ", "),
      absl::Substitute(R"hdr(#include "transpiler/yosys_$0runner.h")hdr",
                       transpiler_type == "yosys_plaintext"
                           ? "plaintext_"
                           : "interpreted_tfhe_"),
      transpiler_type == "yosys_plaintext" ? "" : "Tfhe",
      transpiler_type == "yosys_plaintext" ? "" : ", bk");
}

absl::StatusOr<std::string> YosysTranspiler::TranslateHeader(
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view header_path, const absl::string_view transpiler_type) {
  XLS_ASSIGN_OR_RETURN(const std::string header_guard,
                       PathToHeaderGuard(header_path, transpiler_type));
  static constexpr absl::string_view kHeaderTemplate =
      R"(#ifndef $1
#define $1

#include "absl/status/status.h"
#include "absl/types/span.h"

$2

$0;

#endif  // $1
)";
  XLS_ASSIGN_OR_RETURN(std::string signature,
                       FunctionSignature(metadata, transpiler_type));
  return absl::Substitute(kHeaderTemplate, signature, header_guard,
                          transpiler_type == "yosys_plaintext" ? "" : R"hdr(
#include "transpiler/data/tfhe_data.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
                          )hdr");
}

absl::StatusOr<std::string> YosysTranspiler::FunctionSignature(
    const xlscc_metadata::MetadataOutput& metadata,
    const absl::string_view transpiler_type) {
  std::vector<std::string> param_signatures;
  std::string type =
      transpiler_type == "yosys_plaintext" ? "bool" : "LweSample";
  if (!metadata.top_func_proto().return_type().has_as_void()) {
    param_signatures.push_back(absl::Substitute("absl::Span<$0> result", type));
  }
  for (auto& param : metadata.top_func_proto().params()) {
    param_signatures.push_back(
        absl::StrCat(absl::Substitute("absl::Span<$0> ", type), param.name()));
  }

  if (transpiler_type == "yosys_interpreted_tfhe") {
    param_signatures.push_back("const TFheGateBootstrappingCloudKeySet* bk");
  }

  std::string function_name = metadata.top_func_proto().name().name();
  return absl::Substitute("absl::Status $0($1)", function_name,
                          absl::StrJoin(param_signatures, ", "));
}

absl::StatusOr<std::string> YosysTranspiler::PathToHeaderGuard(
    absl::string_view header_path, const absl::string_view transpiler_type) {
  std::string stem = absl::Substitute("YOSYS_$0GENERATE_H_",
                                      transpiler_type == "yosys_plaintext"
                                          ? "PLAINTEXT_"
                                          : "INTERPRETED_TFHE_");
  if (header_path == "-") return stem;
  std::string header_guard = absl::AsciiStrToUpper(header_path);
  std::transform(header_guard.begin(), header_guard.end(), header_guard.begin(),
                 [](unsigned char c) -> unsigned char {
                   if (std::isalnum(c)) {
                     return c;
                   } else {
                     return '_';
                   }
                 });
  return absl::StrCat(stem, header_guard);
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
