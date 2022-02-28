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

#include "transpiler/palisade_transpiler.h"

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
#include "xls/ir/function.h"
#include "xls/ir/node.h"
#include "xls/ir/node_iterator.h"
#include "xls/ir/nodes.h"
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
  return absl::StrFormat("%s[%d]", param->GetName(), offset);
}

std::string OutputBitReference(absl::string_view output_arg, int offset) {
  return absl::StrFormat("%s[%d]", output_arg, offset);
}

std::string CopyTo(std::string destination, std::string source) {
  return absl::Substitute("  $0 = $1;\n", destination, source);
}

}  // namespace

// Input: "result", 0, Node(id = 3)
// Output: result[0] = temp_nodes[3];
std::string PalisadeTranspiler::CopyNodeToOutput(absl::string_view output_arg,
                                                 int offset,
                                                 const xls::Node* node) {
  return CopyTo(OutputBitReference(output_arg, offset), NodeReference(node));
}

// Input: Node(id = 4), Param("some_param"), 3
// Output: temp_nodes[4] = some_param[3];
std::string PalisadeTranspiler::CopyParamToNode(const xls::Node* node,
                                                const xls::Node* param,
                                                int offset) {
  return CopyTo(NodeReference(node), ParamBitReference(param, offset));
}

// No-op
std::string PalisadeTranspiler::InitializeNode(const Node* node) { return ""; }

// clang-format off
//
// Input: Node(id = 5, op = kNot, operands = {Node(id = 2)})
// Output: "  temp_nodes[5] = cc.EvalNOT(temp_nodes[2]);\n"
//
// Input: Node(id = 5, op = kAnd, operands = {Node(id = 2), Node(id = 3)})
// Output: "  temp_nodes[5] = cc.EvalBinGate(lbcrypto::AND, temp_nodes[2], temp_nodes[3]);\n"
//
// clang-format on
absl::StatusOr<std::string> PalisadeTranspiler::Execute(const Node* node) {
  absl::ParsedFormat<'s', 's'> statement{
      "// Unsupported Op kind; arguments \"%s\", \"%s\"\n\n"};
  switch (node->op()) {
    case Op::kAnd:
      statement = absl::ParsedFormat<'s', 's'>{
          "  %s = EvalBinGate(cc, lbcrypto::AND, %s);\n\n"};
      break;
    case Op::kOr:
      statement = absl::ParsedFormat<'s', 's'>{
          "  %s = EvalBinGate(cc, lbcrypto::OR, %s);\n\n"};
      break;
    case Op::kNot:
      statement = absl::ParsedFormat<'s', 's'>{"  %s = cc.EvalNOT(%s);\n\n"};
      break;
    case Op::kLiteral:
      statement =
          absl::ParsedFormat<'s', 's'>{"  %s = cc.EvalConstant(%s);\n\n"};
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

absl::StatusOr<std::string> PalisadeTranspiler::TranslateHeader(
    const xls::Function* function,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view header_path) {
  const std::string header_guard =
      transpiler::PathToHeaderGuard("PALISADE_GENERATE_H_", header_path);
  static constexpr absl::string_view kHeaderTemplate =
      R"(#ifndef $1
#define $1

// clang-format off
#include "$2"
// clang-format on
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "palisade/binfhe/binfhecontext.h"
#include "transpiler/data/palisade_data.h"

$0;

$3#endif  // $1
)";
  absl::optional<std::string> typed_overload =
      TypedOverload(metadata, "Palisade", "absl::Span<lbcrypto::LWECiphertext>",
                    "lbcrypto::BinFHEContext");
  XLS_ASSIGN_OR_RETURN(std::string signature,
                       FunctionSignature(function, metadata));
  return absl::Substitute(kHeaderTemplate, signature, header_guard,
                          GetTypeHeader(header_path),
                          typed_overload.value_or(""));
}

absl::StatusOr<std::string> PalisadeTranspiler::FunctionSignature(
    const Function* function, const xlscc_metadata::MetadataOutput& metadata) {
  return transpiler::FunctionSignature(metadata, "lbcrypto::LWECiphertext",
                                       "lbcrypto::BinFHEContext", "cc");
}

absl::StatusOr<std::string> PalisadeTranspiler::Prelude(
    const Function* function, const xlscc_metadata::MetadataOutput& metadata) {
  // $0: function name
  // $1: non-key parameter string
  static constexpr absl::string_view kPrelude =
      R"(#include <unordered_map>

#include "absl/status/status.h"
#include "absl/types/span.h"
#include "transpiler/common_runner.h"
#include "palisade/binfhe/binfhecontext.h"
#include "palisade/binfhe/ringcore.h"

static StructReverseEncodeOrderSetter ORDER;

static inline lbcrypto::LWECiphertext EvalBinGate(
    lbcrypto::BinFHEContext cc, const lbcrypto::BINGATE gate,
    const lbcrypto::LWECiphertext ct1, const lbcrypto::LWECiphertext ct2) {
  if (ct1 == ct2) {
    if (gate == lbcrypto::OR || gate == lbcrypto::AND) {
      return ct1;
    } else if (gate == lbcrypto::NOR || gate == lbcrypto::NAND) {
      return cc.EvalNOT(ct1);
    } else if (gate == lbcrypto::XOR || gate == lbcrypto::XOR_FAST) {
      return cc.EvalConstant(0);
    } else if (gate == lbcrypto::XNOR || gate == lbcrypto::XNOR_FAST) {
      return cc.EvalConstant(1);
    }
  }
  return cc.EvalBinGate(gate, ct1, ct2);
}

$0 {
  std::unordered_map<int, lbcrypto::LWECiphertext> temp_nodes;

)";
  XLS_ASSIGN_OR_RETURN(std::string signature,
                       FunctionSignature(function, metadata));
  return absl::Substitute(kPrelude, signature);
}

absl::StatusOr<std::string> PalisadeTranspiler::Conclusion() {
  return R"(  return absl::OkStatus();
}
)";
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
