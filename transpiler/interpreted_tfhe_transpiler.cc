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

#include "transpiler/interpreted_tfhe_transpiler.h"

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
#include "xls/common/status/status_macros.h"
#include "xls/ir/function.h"
#include "xls/ir/node.h"
#include "xls/ir/node_iterator.h"
#include "xls/ir/nodes.h"
#include "xls/public/value.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

absl::StatusOr<std::string> InterpretedTfheTranspiler::Translate(
    const xls::Function* function,
    const xlscc_metadata::MetadataOutput& metadata) {
  static constexpr absl::string_view kSourceTemplate =
      R"(#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
#include "transpiler/tfhe_runner.h"
#include "xls/common/status/status_macros.h"

namespace {

static constexpr char kXLSPackage[] = R"ir(
$0
)ir";

static constexpr char kFunctionMetadata[] = R"pb(
$1
)pb";

using fully_homomorphic_encryption::transpiler::TfheRunner;

}  // namespace

$2 {
  XLS_ASSIGN_OR_RETURN(auto runner, TfheRunner::CreateFromStrings(
                                    kXLSPackage, kFunctionMetadata));
  return runner->Run($3, {$4}, bk);
}
)";
  XLS_ASSIGN_OR_RETURN(const std::string signature,
                       FunctionSignature(function, metadata));
  std::string return_param = "{}";
  if (!metadata.top_func_proto().return_type().has_as_void()) {
    return_param = "result";
  }
  std::vector<std::string> param_entries;
  for (xls::Param* param : function->params()) {
    param_entries.push_back(absl::Substitute(R"({"$0", $0})", param->name()));
  }

  // Serialize the metadata, removing the trailing null.
  std::string metadata_text;
  XLS_CHECK(
      google::protobuf::TextFormat::PrintToString(metadata, &metadata_text));

  return absl::Substitute(kSourceTemplate, function->package()->DumpIr(),
                          metadata_text, signature, return_param,
                          absl::StrJoin(param_entries, ", "));
}

absl::StatusOr<std::string> InterpretedTfheTranspiler::TranslateHeader(
    const xls::Function* function,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view header_path) {
  XLS_ASSIGN_OR_RETURN(const std::string header_guard,
                       PathToHeaderGuard(header_path));
  static constexpr absl::string_view kHeaderTemplate =
      R"(#ifndef $1
#define $1

#include "absl/status/status.h"
#include "absl/types/span.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"

$0;
#endif  // $1
)";
  XLS_ASSIGN_OR_RETURN(std::string signature,
                       FunctionSignature(function, metadata));
  return absl::Substitute(kHeaderTemplate, signature, header_guard);
}

absl::StatusOr<std::string> InterpretedTfheTranspiler::FunctionSignature(
    const xls::Function* function,
    const xlscc_metadata::MetadataOutput& metadata) {
  std::vector<std::string> param_signatures;
  if (!metadata.top_func_proto().return_type().has_as_void()) {
    param_signatures.push_back("absl::Span<LweSample> result");
  }
  for (xls::Param* param : function->params()) {
    param_signatures.push_back(
        absl::StrCat("absl::Span<LweSample> ", param->name()));
  }

  constexpr absl::string_view key_param =
      "const TFheGateBootstrappingCloudKeySet* bk";
  if (param_signatures.empty()) {
    return absl::Substitute("absl::Status $0($1)", function->name(), key_param);
  } else {
    return absl::Substitute("absl::Status $0($1,\n  $2)", function->name(),
                            absl::StrJoin(param_signatures, ", "), key_param);
  }
}

absl::StatusOr<std::string> InterpretedTfheTranspiler::PathToHeaderGuard(
    absl::string_view header_path) {
  if (header_path == "-") return "FHE_GENERATE_H_";
  std::string header_guard = "";
  std::vector<absl::string_view> sub_paths = absl::StrSplit(header_path, '/');
  for (auto c : sub_paths[sub_paths.size() - 1]) {
    header_guard += std::isalnum(c) ? std::toupper(c) : '_';
  }
  if (header_guard.empty()) {
    return absl::InvalidArgumentError("Invalid header_path");
  }
  header_guard += '_';
  return header_guard;
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
