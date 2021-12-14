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

}  // namespace

std::string TfheTranspiler::NodeReference(const Node* node) {
  return absl::StrFormat("temp_nodes[%d]", node->id());
}

std::string TfheTranspiler::ParamBitReference(const Node* param, int offset) {
  int64_t param_bits = param->GetType()->GetFlatBitCount();
  if (param_bits == 1) {
    if (param->Is<xls::TupleIndex>() || param->Is<xls::ArrayIndex>()) {
      return param->operand(0)->GetName();
    }
  }
  return absl::StrFormat("&%s[%d]", param->GetName(), offset);
}

std::string TfheTranspiler::OutputBitReference(absl::string_view output_arg,
                                               int offset) {
  return absl::StrFormat("&%s[%d]", output_arg, offset);
}

std::string TfheTranspiler::CopyTo(std::string destination,
                                   std::string source) {
  return absl::Substitute("  bootsCOPY($0, $1, bk);\n", destination, source);
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
  static const absl::flat_hash_map<xls::Op, absl::string_view> kFHEOps = {
      {Op::kAnd, "bootsAND"},
      {Op::kOr, "bootsOR"},
      {Op::kNot, "bootsNOT"},
      {Op::kLiteral, "bootsCONSTANT"},
  };
  auto it = kFHEOps.find(node->op());
  if (it == kFHEOps.end()) {
    return absl::InvalidArgumentError("Unsupported Op kind.");
  }
  const absl::string_view tfhe_op = it->second;

  std::string operation =
      absl::StrFormat("  %s(%s, ", tfhe_op, NodeReference(node));
  if (node->Is<Literal>()) {
    const Literal* literal = node->As<Literal>();
    XLS_ASSIGN_OR_RETURN(const Bits bit, literal->value().GetBitsWithStatus());
    if (bit.IsOne()) {
      absl::StrAppend(&operation, "1, ");
    } else if (bit.IsZero()) {
      absl::StrAppend(&operation, "0, ");
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
      absl::StrAppend(&operation, NodeReference(operand_node), ", ");
    }
  }
  // bk is the bootstrapping key from TFHE library.
  absl::StrAppend(&operation, "bk);\n\n");
  return operation;
}

absl::StatusOr<std::string> TfheTranspiler::TranslateHeader(
    const xls::Function* function,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view header_path) {
  XLS_ASSIGN_OR_RETURN(const std::string header_guard,
                       PathToHeaderGuard(header_path));
  static constexpr absl::string_view kHeaderTemplate =
      R"(#ifndef $1
#define $1

#include "absl/status/status.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"

$0;
#endif  // $1
)";
  XLS_ASSIGN_OR_RETURN(std::string signature,
                       FunctionSignature(function, metadata));
  return absl::Substitute(kHeaderTemplate, signature, header_guard);
}

absl::StatusOr<std::string> TfheTranspiler::FunctionSignature(
    const Function* function, const xlscc_metadata::MetadataOutput& metadata) {
  std::vector<std::string> param_signatures;
  if (!metadata.top_func_proto().return_type().has_as_void()) {
    param_signatures.push_back("LweSample* result");
  }
  for (Param* param : function->params()) {
    param_signatures.push_back(absl::StrCat("LweSample* ", param->name()));
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

absl::StatusOr<std::string> TfheTranspiler::Prelude(
    const Function* function, const xlscc_metadata::MetadataOutput& metadata) {
  // $0: function name
  // $1: non-key parameter string
  static constexpr absl::string_view kPrelude =
      R"(#include <unordered_map>

#include "absl/status/status.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"

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
