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

#include "transpiler/interpreted_palisade_transpiler.h"

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
#include "transpiler/common_transpiler.h"
#include "xls/common/status/status_macros.h"
#include "xls/ir/function.h"
#include "xls/ir/node.h"
#include "xls/ir/node_iterator.h"
#include "xls/ir/nodes.h"
#include "xls/public/value.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

absl::StatusOr<std::string> InterpretedPalisadeTranspiler::Translate(
    const xls::Function* function,
    const xlscc_metadata::MetadataOutput& metadata) {
  static constexpr absl::string_view kSourceTemplate =
      R"(#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "transpiler/palisade_runner.h"
#include "palisade/binfhe/binfhecontext.h"
#include "xls/common/status/status_macros.h"

namespace {

static constexpr char kXLSPackage[] = R"ir(
$0
)ir";

static constexpr char kFunctionMetadata[] = R"pb(
$1
)pb";

using fully_homomorphic_encryption::transpiler::PalisadeRunner;

}  // namespace

$2 {
  XLS_ASSIGN_OR_RETURN(auto runner, PalisadeRunner::CreateFromStrings(
                                    kXLSPackage, kFunctionMetadata));
  return runner->Run($3, {$4}, cc);
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

absl::StatusOr<std::string> InterpretedPalisadeTranspiler::TranslateHeader(
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
#include "palisade/binfhe/binfhecontext.h"

$0;
#endif  // $1
)";
  XLS_ASSIGN_OR_RETURN(std::string signature,
                       FunctionSignature(function, metadata));
  return absl::Substitute(kHeaderTemplate, signature, header_guard);
}

absl::StatusOr<std::string> InterpretedPalisadeTranspiler::FunctionSignature(
    const xls::Function* function,
    const xlscc_metadata::MetadataOutput& metadata) {
  return transpiler::FunctionSignature(metadata, "lbcrypto::LWECiphertext",
                                       "lbcrypto::BinFHEContext", "cc");
}

absl::StatusOr<std::string> InterpretedPalisadeTranspiler::PathToHeaderGuard(
    absl::string_view header_path) {
  return transpiler::PathToHeaderGuard("PALISADE_GENERATE_H_", header_path);
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
