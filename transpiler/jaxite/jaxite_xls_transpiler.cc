#include "transpiler/jaxite/jaxite_xls_transpiler.h"

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "transpiler/util/string.h"
#include "xls/public/ir.h"

namespace fhe {
namespace jaxite {
namespace transpiler {

namespace {

using ::fully_homomorphic_encryption::ToSnakeCase;
using ::xls::Bits;
using ::xls::Function;
using ::xls::Literal;
using ::xls::Node;
using ::xls::Op;
using ::xls::Param;

}  // namespace

// Input: "result", 0, Node(id = 3)
// Output: result[0] = temp_nodes[3]
std::string JaxiteXlsTranspiler::CopyNodeToOutput(absl::string_view output_arg,
                                                  int offset,
                                                  const xls::Node* node) {
  return absl::StrFormat("  %s[%d] = temp_nodes[%d]\n", output_arg, offset,
                         node->id());
}

// Input: Node(id = 4), Param("some_param"), 3
// Output: temp_nodes[4] = some_param[3]
std::string JaxiteXlsTranspiler::CopyParamToNode(const xls::Node* node,
                                                 const xls::Node* param,
                                                 int offset) {
  return absl::StrFormat("  temp_nodes[%d] = %s[%d]\n", node->id(),
                         param->GetName(), offset);
}

// No-Op
std::string JaxiteXlsTranspiler::InitializeNode(const Node* node) { return ""; }

absl::StatusOr<std::string> JaxiteXlsTranspiler::Execute(const Node* node) {
  absl::ParsedFormat<'d', 's'> statement{
      "// Unsupported Op kind; arguments \"%d\", \"%s\"\n\n"};
  switch (node->op()) {
    case Op::kAnd:
      statement = absl::ParsedFormat<'d', 's'>{
          "  temp_nodes[%d] = jaxite_bool.and_(%s, sks, params)\n\n"};
      break;
    case Op::kOr:
      statement = absl::ParsedFormat<'d', 's'>{
          "  temp_nodes[%d] = jaxite_bool.or_(%s, sks, params)\n\n"};
      break;
    case Op::kNot:
      statement = absl::ParsedFormat<'d', 's'>{
          "  temp_nodes[%d] = jaxite_bool.not_(%s, params)\n\n"};
      break;
    case Op::kLiteral:
      statement = absl::ParsedFormat<'d', 's'>{
          "  temp_nodes[%d] = jaxite_bool.constant(%s, params)\n\n"};
      break;
    default:
      return absl::InvalidArgumentError(
          absl::StrCat("Unsupported Op kind: ", xls::OpToString(node->op())));
  }

  std::vector<std::string> arguments;
  if (node->Is<Literal>()) {
    const Literal* literal = node->As<Literal>();
    XLS_ASSIGN_OR_RETURN(const Bits bit, literal->value().GetBitsWithStatus());
    if (bit.IsOne()) {
      arguments.push_back("True");
    } else if (bit.IsZero()) {
      arguments.push_back("False");
    } else {
      // We allow literals strictly for pulling values out of [param] arrays.
      for (const Node* user : literal->users()) {
        if (!user->Is<xls::ArrayIndex>()) {
          return absl::InvalidArgumentError(
              absl::StrCat("Unsupported literal argument of type: ",
                           xls::TypeKindToString(user->GetType()->kind())));
        }
      }
      return "";
    }
  } else {
    for (const Node* operand_node : node->operands()) {
      arguments.push_back(
          absl::StrFormat("temp_nodes[%d]", operand_node->id()));
    }
  }
  return absl::StrFormat(statement, node->id(), absl::StrJoin(arguments, ", "));
}

// No-Op
absl::StatusOr<std::string> JaxiteXlsTranspiler::TranslateHeader(
    const xls::Function* function,
    const xlscc_metadata::MetadataOutput& metadata,
    absl::string_view header_path) {
  return "";
}

absl::StatusOr<std::string> JaxiteXlsTranspiler::FunctionSignature(
    const Function* function, const xlscc_metadata::MetadataOutput& metadata) {
  std::vector<std::string> param_signatures;
  if (!metadata.top_func_proto().return_type().has_as_void()) {
    param_signatures.push_back("result: List[types.LweCiphertext]");
  }
  for (const Param* param : function->params()) {
    param_signatures.push_back(
        absl::StrCat(param->name(), ": List[types.LweCiphertext]"));
  }

  constexpr absl::string_view server_key_set = "sks: jaxite_bool.ServerKeySet";
  constexpr absl::string_view parameters = "params: jaxite_bool.Parameters";
  return absl::Substitute(
      "def $0($1, $2, $3) -> None", ToSnakeCase(function->name()),
      absl::StrJoin(param_signatures, ", "), server_key_set, parameters);
}

absl::StatusOr<std::string> JaxiteXlsTranspiler::Prelude(
    const Function* function, const xlscc_metadata::MetadataOutput& metadata) {
  // $0: function signature
  static constexpr absl::string_view kPrelude =
      R"(from typing import Dict, List

from jaxite.jaxite_bool import jaxite_bool
from jaxite.jaxite_lib import types

$0:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
)";
  XLS_ASSIGN_OR_RETURN(std::string signature,
                       FunctionSignature(function, metadata));
  return absl::Substitute(kPrelude, signature);
}

// No-op
absl::StatusOr<std::string> JaxiteXlsTranspiler::Conclusion() { return ""; }

}  // namespace transpiler
}  // namespace jaxite
}  // namespace fhe
