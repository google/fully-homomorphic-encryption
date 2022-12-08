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

#include "transpiler/tfhe_transpiler.h"

#include <string>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "transpiler/common_transpiler.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/public/ir.h"
#include "xls/public/value.h"

namespace xls {
class Node;
}  // namespace xls

namespace fully_homomorphic_encryption {
namespace transpiler {

namespace {

using xls::Bits;
using xls::Function;
using xls::Literal;
using xls::Node;
using xls::Op;
using xls::Param;

std::string NodeReference(const Node* node) {
  return absl::StrFormat("temp_nodes[%d]", node->id());
}

std::string ParamBitReference(const Node* param, int offset) {
  int64_t param_bits = param->GetType()->GetFlatBitCount();
  if (param_bits == 1) {
    if (param->Is<xls::TupleIndex>() || param->Is<xls::ArrayIndex>()) {
      return param->operand(0)->GetName();
    }
  }
  return absl::StrFormat("&%s[%d]", param->GetName(), offset);
}

std::string OutputBitReference(absl::string_view output_arg, int offset) {
  return absl::StrFormat("&%s[%d]", output_arg, offset);
}

std::string CopyTo(absl::string_view destination, absl::string_view source) {
  return absl::Substitute("  bootsCOPY($0, $1, bk);\n", destination, source);
}

}  // namespace

// Input: "result", 0, Node(id = 3)
// Output: result[0] = temp_nodes[3];
std::string TfheTranspiler::CopyNodeToOutput(absl::string_view output_arg,
                                             int offset,
                                             const xls::Node* node) {
  return CopyTo(OutputBitReference(output_arg, offset), NodeReference(node));
}

// Input: Node(id = 4), Param("some_param"), 3
// Output: temp_nodes[4] = &some_param[3];
std::string TfheTranspiler::CopyParamToNode(const xls::Node* node,
                                            const xls::Node* param,
                                            int offset) {
  return CopyTo(NodeReference(node), ParamBitReference(param, offset));
}

// Input: Node(id = 5)
// Output: temp_nodes[5] = new new_gate_bootstrapping_ciphertext(bk->params);
std::string TfheTranspiler::InitializeNode(const Node* node) {
  return absl::Substitute(
      "  $0 = new_gate_bootstrapping_ciphertext(bk->params);\n",
      NodeReference(node));
}

// Input: Node(id = 5, op = kNot, operands = Node(id = 2))
// Output: "  bootsNOT(temp_nodes[5], temp_nodes[2], bk);\n\n"
absl::StatusOr<std::string> TfheTranspiler::Execute(const Node* node) {
  absl::ParsedFormat<'s', 's'> statement{
      "// Unsupported Op kind; arguments \"%s\", \"%s\"\n\n"};
  switch (node->op()) {
    case Op::kAnd:
      statement = absl::ParsedFormat<'s', 's'>{"  bootsAND(%s, %s, bk);\n\n"};
      break;
    case Op::kOr:
      statement = absl::ParsedFormat<'s', 's'>{"  bootsOR(%s, %s, bk);\n\n"};
      break;
    case Op::kNot:
      statement = absl::ParsedFormat<'s', 's'>{"  bootsNOT(%s, %s, bk);\n\n"};
      break;
    case Op::kLiteral:
      statement =
          absl::ParsedFormat<'s', 's'>{"  bootsCONSTANT(%s, %s, bk);\n\n"};
      break;
    default:
      return absl::InvalidArgumentError("Unsupported Op kind.");
  }

  std::vector<std::string> arguments;
  if (node->Is<Literal>()) {
    const Literal* literal = node->As<Literal>();
    XLS_ASSIGN_OR_RETURN(const Bits bit, literal->value().GetBitsWithStatus());
    if (bit.IsOne()) {
      arguments.push_back("1");
    } else if (bit.IsZero()) {
      arguments.push_back("0");
    } else {
      // We allow literals strictly for pulling values out of [param] arrays.
      for (const Node* user : literal->users()) {
        if (!user->Is<xls::ArrayIndex>()) {
          return absl::InvalidArgumentError("Unsupported literal value.");
        }
      }
      return "";
    }
  } else {
    for (const Node* operand_node : node->operands()) {
      arguments.push_back(NodeReference(operand_node));
    }
  }
  return absl::StrFormat(statement, NodeReference(node),
                         absl::StrJoin(arguments, ", "));
}

absl::StatusOr<std::string> TfheTranspiler::TranslateHeader(
    const xls::Function* function,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view header_path,
    const absl::string_view encryption_specific_transpiled_structs_header_path,
    bool skip_scheme_data_deps, const std::vector<std::string>& unwrap) {
  XLS_ASSIGN_OR_RETURN(const std::string header_guard,
                       PathToHeaderGuard(header_path));
  static constexpr absl::string_view kHeaderTemplate =
      R"(#ifndef $2
#define $2

#include "$3"
#include "absl/status/status.h"
#include "absl/types/span.h"
$4
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"

$0;

$1#endif  // $2
)";
  XLS_ASSIGN_OR_RETURN(std::string signature,
                       FunctionSignature(function, metadata));
  absl::optional<std::string> typed_overload =
      TypedOverload(metadata, "Tfhe", "absl::Span<LweSample>",
                    "const TFheGateBootstrappingCloudKeySet*", "bk", unwrap);
  return absl::Substitute(
      kHeaderTemplate, signature, typed_overload.value_or(""), header_guard,
      encryption_specific_transpiled_structs_header_path,
      skip_scheme_data_deps ? "" : R"(#include "transpiler/data/tfhe_data.h")");
}

absl::StatusOr<std::string> TfheTranspiler::FunctionSignature(
    const Function* function, const xlscc_metadata::MetadataOutput& metadata) {
  return transpiler::FunctionSignature(
      metadata, "LweSample", "const TFheGateBootstrappingCloudKeySet*", "bk");
}

absl::StatusOr<std::string> TfheTranspiler::Prelude(
    const Function* function, const xlscc_metadata::MetadataOutput& metadata) {
  // $0: function name
  // $1: non-key parameter string
  static constexpr absl::string_view kPrelude =
      R"(#include <unordered_map>

#include "absl/status/status.h"
#include "absl/types/span.h"
#include "transpiler/common_runner.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"

static StructReverseEncodeOrderSetter ORDER;

$0 {
  std::unordered_map<int, LweSample*> temp_nodes;

)";
  XLS_ASSIGN_OR_RETURN(std::string signature,
                       FunctionSignature(function, metadata));
  return absl::Substitute(kPrelude, signature);
}

absl::StatusOr<std::string> TfheTranspiler::Conclusion() {
  return R"(  for (auto pair : temp_nodes) {
    delete_gate_bootstrapping_ciphertext(pair.second);
  }
  return absl::OkStatus();
}
)";
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
