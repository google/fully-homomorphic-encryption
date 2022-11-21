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
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/text_format.h"
#include "transpiler/common_transpiler.h"
#include "transpiler/pipeline_enums.h"
#include "xls/common/logging/logging.h"
#include "xls/common/status/status_macros.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

namespace {

absl::StatusOr<std::string> ElementType(Encryption encryption) {
  switch (encryption) {
    case Encryption::kCleartext:
      return "bool";
    case Encryption::kTFHE:
      return "LweSample";
    case Encryption::kOpenFHE:
      return "lbcrypto::LWECiphertext";
    default:
      return absl::UnimplementedError(absl::Substitute(
          "The Yosys pipeline does not currently implement '$0' encryption.",
          AbslUnparseFlag(encryption)));
  }
}

absl::StatusOr<std::string> DataType(Encryption encryption) {
  XLS_ASSIGN_OR_RETURN(std::string element_type, ElementType(encryption));
  return absl::Substitute("absl::Span<$0>", element_type);
}

}  // namespace

absl::StatusOr<std::string> YosysTranspiler::Translate(
    const xlscc_metadata::MetadataOutput& metadata,
    const absl::string_view cell_library_text,
    const absl::string_view netlist_text, Encryption encryption) {
  static constexpr absl::string_view kSourceTemplate =
      R"source(#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "transpiler/common_runner.h"
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

static StructDeclarationEncodeOrderSetter ORDER;
static fully_homomorphic_encryption::transpiler::Yosys$7Runner runner(
                            kCellDefinitions,
                            kNetlistPackage,
                            kFunctionMetadata);

}  // namespace

$3 {
  return runner.Run($4, {$5}, {$9}$8);
})source";

  XLS_ASSIGN_OR_RETURN(const std::string signature,
                       FunctionSignature(metadata, encryption));
  XLS_ASSIGN_OR_RETURN(const std::string data_type, DataType(encryption));
  std::string return_param = absl::Substitute("$0()", data_type);

  if (!metadata.top_func_proto().return_type().has_as_void()) {
    return_param = "result";
  }
  std::vector<std::string> in_param_entries;
  std::vector<std::string> inout_param_entries;
  for (auto& param : metadata.top_func_proto().params()) {
    if (param.is_reference() && !param.is_const()) {
      inout_param_entries.push_back(param.name());
    } else {
      in_param_entries.push_back(param.name());
    }
  }

  // Serialize the metadata, removing the trailing null.
  std::string metadata_text;
  XLS_CHECK(
      google::protobuf::TextFormat::PrintToString(metadata, &metadata_text));

  std::string runner_prefix;
  std::string args_suffix;
  switch (encryption) {
    case Encryption::kTFHE:
      runner_prefix = "Tfhe";
      args_suffix = ", bk";
      break;
    case Encryption::kOpenFHE:
      runner_prefix = "OpenFhe";
      args_suffix = ", cc";
      break;
    case Encryption::kCleartext:
      // Uses YosysRunner; no prefix needed.
      // No key argument required.
      break;
    default:
      return absl::UnimplementedError(absl::Substitute(
          "The Yosys pipeline does not currently implement '$0' encryption.",
          AbslUnparseFlag(encryption)));
  }

  return absl::Substitute(
      kSourceTemplate, netlist_text, metadata_text, cell_library_text,
      signature, return_param, absl::StrJoin(in_param_entries, ", "),
      absl::Substitute(R"hdr(#include "transpiler/yosys_$0_runner.h")hdr",
                       AbslUnparseFlag(encryption)),
      runner_prefix, args_suffix, absl::StrJoin(inout_param_entries, ", "));
}

absl::StatusOr<std::string> YosysTranspiler::TranslateHeader(
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view header_path, Encryption encryption,
    const absl::string_view encryption_specific_transpiled_structs_header_path,
    const std::vector<std::string>& unwrap) {
  XLS_ASSIGN_OR_RETURN(const std::string header_guard,
                       PathToHeaderGuard(header_path, encryption));
  static constexpr absl::string_view kHeaderTemplate =
      R"(#ifndef $1
#define $1

#include "absl/status/status.h"
#include "absl/types/span.h"
$2

$0;

$3#endif  // $1
)";
  XLS_ASSIGN_OR_RETURN(std::string signature,
                       FunctionSignature(metadata, encryption));

  XLS_ASSIGN_OR_RETURN(std::string data_type, DataType(encryption));

  absl::optional<std::string> typed_overload;
  std::string types_include;
  std::string scheme_data_header;
  switch (encryption) {
    case Encryption::kTFHE:
      typed_overload = TypedOverload(metadata, "Tfhe", data_type,
                                     "const TFheGateBootstrappingCloudKeySet*",
                                     "bk", unwrap);
      scheme_data_header = R"hdr(
#include "transpiler/data/tfhe_data.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
)hdr";
      break;
    case Encryption::kOpenFHE:
      typed_overload = TypedOverload(metadata, "OpenFhe", data_type,
                                     "lbcrypto::BinFHEContext", "bk", unwrap);
      scheme_data_header = R"hdr(
#include "transpiler/data/openfhe_data.h"
#include "openfhe/binfhe/binfhecontext.h"
)hdr";
      break;
    case Encryption::kCleartext:
      typed_overload = TypedOverload(metadata, "Encoded", data_type,
                                     absl::nullopt, "", unwrap);
      scheme_data_header = R"hdr(
#include "transpiler/data/cleartext_data.h"
)hdr";
      break;
    default:
      return absl::UnimplementedError(absl::Substitute(
          "The Yosys pipeline does not currently implement '$0' encryption.",
          AbslUnparseFlag(encryption)));
  }

  types_include = absl::Substitute(
      R"hdr(
#include "$0"
$1
                          )hdr",
      encryption_specific_transpiled_structs_header_path, scheme_data_header);

  return absl::Substitute(kHeaderTemplate, signature, header_guard,
                          types_include, typed_overload.value_or(""));
}

absl::StatusOr<std::string> YosysTranspiler::FunctionSignature(
    const xlscc_metadata::MetadataOutput& metadata, Encryption encryption) {
  XLS_ASSIGN_OR_RETURN(const std::string element_type, ElementType(encryption));
  switch (encryption) {
    case Encryption::kTFHE:
      return transpiler::FunctionSignature(
          metadata, element_type, "const TFheGateBootstrappingCloudKeySet*",
          "bk");
    case Encryption::kOpenFHE:
      return transpiler::FunctionSignature(metadata, element_type,
                                           "lbcrypto::BinFHEContext", "cc");
    case Encryption::kCleartext:
      return transpiler::FunctionSignature(metadata, element_type,
                                           absl::nullopt);
    default:
      return absl::UnimplementedError(absl::Substitute(
          "The Yosys pipeline does not currently implement '$0' encryption.",
          AbslUnparseFlag(encryption)));
  }
}

absl::StatusOr<std::string> YosysTranspiler::PathToHeaderGuard(
    absl::string_view header_path, Encryption encryption) {
  std::string stem =
      absl::Substitute("YOSYS_$0_GENERATE_H_",
                       absl::AsciiStrToUpper(AbslUnparseFlag(encryption)));
  return transpiler::PathToHeaderGuard(stem, header_path);
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
