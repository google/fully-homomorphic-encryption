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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_ABSTRACT_XLS_TRANSPILER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_ABSTRACT_XLS_TRANSPILER_H_

#include <stdint.h>

#include <cctype>
#include <string>
#include <type_traits>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "transpiler/common_transpiler.h"
#include "xls/common/logging/logging.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/protected/ir.h"
#include "xls/public/ir.h"

namespace xlscc_metadata {

// We need to know the number of out params when collecting outputs, because
// otherwise we can't tell the difference between returning one three-element
// tuple or three one-element quantities.
static inline int GetNumOutParams(const MetadataOutput& metadata) {
  int num_out_params = 0;
  if (!metadata.top_func_proto().return_type().has_as_void()) {
    num_out_params++;
  }

  for (const auto& param : metadata.top_func_proto().params()) {
    if (!param.is_const() && param.is_reference()) {
      num_out_params++;
    }
  }

  return num_out_params;
}

}  // namespace xlscc_metadata

namespace fully_homomorphic_encryption {
namespace transpiler {

// Abstract base for transpilers converting booleanified XLS functions into
// other languages.
//
// The AbstractXLSTranspiler uses the curiously recurring template pattern to
// allow overriding static functions. Classes should be derived like so:
//
//   class FooTranspiler : public AbstractXLSTranspiler<FooTranspiler> {
//     ...
//
// And define static methods TranslateHeader, NodeReference, ParamBitReference,
// OutputBitReference, CopyTo, InitializeNode, Execute, Prelude, and Conclusion.
template <typename TranspilerT>
class AbstractXLSTranspiler {
 public:
  // Takes as input an XLS Function node and expected output and returns an FHE
  // C++ method that uses the gate ops from TFHE library.
  static absl::StatusOr<std::string> Translate(
      const xls::Function* function,
      const xlscc_metadata::MetadataOutput& metadata) {
    XLS_ASSIGN_OR_RETURN(const std::string prelude,
                         Prelude(function, metadata));

    // Generate code implementing each node
    XLS_ASSIGN_OR_RETURN(const std::string body, TranslateNodes(function));

    XLS_ASSIGN_OR_RETURN(const std::string handle_outputs,
                         CollectOutputs(function, metadata));

    XLS_ASSIGN_OR_RETURN(const std::string conclusion, Conclusion());

    return absl::StrCat(prelude, body, handle_outputs, conclusion);
  }

  // Takes as input an XLS Function node and returns an FHE
  // C++ header file.
  static absl::StatusOr<std::string> TranslateHeader(
      const xls::Function* function,
      const xlscc_metadata::MetadataOutput& metadata,
      absl::string_view header_path, bool skip_scheme_data_deps) {
    return TranspilerT::TranslateHeader(function, metadata, header_path,
                                        skip_scheme_data_deps);
  }

  static absl::StatusOr<std::string> PathToHeaderGuard(
      absl::string_view header_path) {
    return transpiler::PathToHeaderGuard("FHE_GENERATE_H_", header_path);
  }

  // Walks the type elements comprising `function`'s output type and generates
  // FHE copy operations to extract the data corresponding to each.
  //
  // At present, `function`'s output must be of the form (A, B), where A is
  // bits- or array-typed, and B must be a tuple containing only bits- or
  // array-typed elements. A corresponds to the output from the original C++
  // function itself, and the elements of B are the in/out params to the
  // function. We don't currently have the ability to traverse the definition of
  // any given [C/C++] struct, so struct/tuple types are not _currently_
  // supported, though this is intended to change in the near future.
  static absl::StatusOr<std::string> CollectOutputs(
      const xls::Function* function,
      const xlscc_metadata::MetadataOutput& metadata) {
    const xls::Node* return_value = function->return_value();

    std::vector<const xls::Node*> elements;
    const xls::Type* type = return_value->GetType();
    int num_out_params = xlscc_metadata::GetNumOutParams(metadata);
    if (type->kind() == xls::TypeKind::kTuple) {
      if (num_out_params != 1) {
        elements.insert(elements.begin(), return_value->operands().begin(),
                        return_value->operands().end());
      } else {
        elements.push_back(return_value);
      }
    } else {
      elements.push_back(return_value);
    }

    if (elements.empty()) {
      return "";
    }

    std::string collected_outputs;

    int output_idx = 0;
    if (!metadata.top_func_proto().return_type().has_as_void()) {
      XLS_ASSIGN_OR_RETURN(
          std::string collected_return,
          CollectNodeValue(elements[output_idx++], "result", 0));
      absl::StrAppend(&collected_outputs, collected_return);
    }

    const auto& fn_params = metadata.top_func_proto().params();
    int param_idx = 0;
    for (; output_idx < elements.size(); output_idx++) {
      const xlscc_metadata::FunctionParameter* param;
      while (true) {
        param = &fn_params[param_idx++];
        if (!param->is_const() && param->is_reference()) {
          break;
        }

        if (param_idx == fn_params.size()) {
          return absl::InternalError(
              absl::StrFormat("No matching in/out param for function param: %s",
                              param->name()));
        }
      }

      XLS_ASSIGN_OR_RETURN(
          std::string collected_param,
          CollectNodeValue(elements[output_idx], param->name(), 0));
      absl::StrAppend(&collected_outputs, collected_param);
    }

    return collected_outputs;
  }

  static std::string CopyNodeToOutput(absl::string_view output_arg, int offset,
                                      const xls::Node* node) {
    return TranspilerT::CopyNodeToOutput(output_arg, offset, node);
  }
  static std::string CopyParamToNode(const xls::Node* node,
                                     const xls::Node* param, int offset) {
    return TranspilerT::CopyParamToNode(node, param, offset);
  }
  static std::string InitializeNode(const xls::Node* node) {
    return TranspilerT::InitializeNode(node);
  }

  static absl::StatusOr<std::string> Execute(const xls::Node* node) {
    return TranspilerT::Execute(node);
  }

  static absl::StatusOr<std::string> Prelude(
      const xls::Function* function,
      const xlscc_metadata::MetadataOutput& metadata) {
    return TranspilerT::Prelude(function, metadata);
  }

  static absl::StatusOr<std::string> Conclusion() {
    return TranspilerT::Conclusion();
  }

 private:
  static absl::StatusOr<int64_t> GetOffsetInArrayIndex(
      const xls::ArrayIndex* array_index) {
    int64_t offset = 0;

    if (array_index->operand(0)->Is<xls::ArrayIndex>()) {
      XLS_ASSIGN_OR_RETURN(offset,
                           GetOffsetInArrayIndex(
                               array_index->operand(0)->As<xls::ArrayIndex>()));
    } else if (array_index->operand(0)->Is<xls::TupleIndex>()) {
      XLS_ASSIGN_OR_RETURN(offset,
                           GetOffsetInTupleIndex(
                               array_index->operand(0)->As<xls::TupleIndex>()));
    }

    XLS_ASSIGN_OR_RETURN(const xls::ArrayType* array_type,
                         array_index->array()->GetType()->AsArray());

    absl::Span<xls::Node* const> indices = array_index->indices();
    for (int i = 0; i < indices.size(); i++) {
      const xls::Type* element_type = array_type->element_type();
      if (!indices[i]->Is<xls::Literal>()) {
        return absl::InvalidArgumentError(
            "Only literal indexes into arrays are supported.");
      }

      const xls::Literal* literal = indices[i]->As<xls::Literal>();
      XLS_ASSIGN_OR_RETURN(int64_t concrete_index,
                           literal->value().bits().ToUint64());
      offset += element_type->GetFlatBitCount() * concrete_index;
    }

    return offset;
  }

  static absl::StatusOr<int64_t> GetOffsetInTupleIndex(
      const xls::TupleIndex* tuple_index) {
    int64_t offset = 0;

    if (tuple_index->operand(0)->Is<xls::ArrayIndex>()) {
      XLS_ASSIGN_OR_RETURN(offset,
                           GetOffsetInArrayIndex(
                               tuple_index->operand(0)->As<xls::ArrayIndex>()));
    } else if (tuple_index->operand(0)->Is<xls::TupleIndex>()) {
      XLS_ASSIGN_OR_RETURN(offset,
                           GetOffsetInTupleIndex(
                               tuple_index->operand(0)->As<xls::TupleIndex>()));
    }

    const xls::TupleType* tuple_type =
        tuple_index->operand(0)->GetType()->AsTupleOrDie();
    for (int64_t i = 0; i < tuple_index->index(); i++) {
      offset += tuple_type->element_type(i)->GetFlatBitCount();
    }

    return offset;
  }

  // This method copies the relevant bit from input params into a temp node.
  static absl::StatusOr<std::string> HandleBitSlice(
      const xls::BitSlice* bit_slice) {
    xls::Node* operand = bit_slice->operand(0);
    int slice_idx = 0;

    if (operand->Is<xls::ArrayIndex>()) {
      const xls::ArrayIndex* array_index = operand->As<xls::ArrayIndex>();
      XLS_ASSIGN_OR_RETURN(slice_idx, GetOffsetInArrayIndex(array_index));
      slice_idx += bit_slice->start();

      while (!operand->Is<xls::Param>()) {
        operand = operand->operand(0);
        // Verify that the only things allowed in a BitSlice chain are array
        // indexes, tuple indexes, other bit slices, and the eventual params.
        XLS_CHECK(operand->Is<xls::ArrayIndex>() ||
                  operand->Is<xls::BitSlice>() || operand->Is<xls::Param>() ||
                  operand->Is<xls::TupleIndex>())
            << "Invalid BitSlice operand: " << operand->ToString();
      }
    } else if (operand->Is<xls::TupleIndex>()) {
      const xls::TupleIndex* tuple_index = operand->As<xls::TupleIndex>();
      XLS_ASSIGN_OR_RETURN(slice_idx, GetOffsetInTupleIndex(tuple_index));
      slice_idx += bit_slice->start();

      while (!operand->Is<xls::Param>()) {
        operand = operand->operand(0);
        // Verify that the only things allowed in a BitSlice chain are array
        // indexes, tuple indexes, other bit slices, and the eventual params.
        XLS_CHECK(operand->Is<xls::ArrayIndex>() ||
                  operand->Is<xls::BitSlice>() || operand->Is<xls::Param>() ||
                  operand->Is<xls::TupleIndex>())
            << "Invalid BitSlice operand: " << operand->ToString();
      }
    } else if (operand->Is<xls::Param>()) {
      slice_idx = bit_slice->start();
    } else {
      return absl::InvalidArgumentError(
          absl::StrCat("Invalid BitSlice operand: ", operand->ToString()));
    }

    // Overflow SHR, can be ignored.
    if (operand->GetType()->GetFlatBitCount() == slice_idx) return "";

    return absl::StrCat(CopyParamToNode(bit_slice, operand, slice_idx), "\n");
  }

  // Array support will need to be updated when structs are added: it could be
  // possible that there is padding present between subsequent elements in an
  // array of these structs that is not captured by the corresponding XLS type -
  // for example, a 56-byte struct will likely be padded out to 64 bytes
  // internally. This code would assume that struct data is all packed, and thus
  // the output would be garbled. Host layout will need to be considered here.
  static absl::StatusOr<std::string> CollectNodeValue(
      const xls::Node* node, absl::string_view output_arg, int output_offset) {
    xls::Type* type = node->GetType();
    std::string outputs;
    switch (type->kind()) {
      case xls::TypeKind::kBits: {
        // If this is a single bit, then we can [finally] emit the copy.
        int64_t bit_count = type->GetFlatBitCount();
        if (bit_count == 1) {
          // We can't handle concats in the transpiler, so if our single-bit is
          // one, walk up a level.
          while (node->Is<xls::Concat>()) {
            node = node->operand(0);
          }
          // Copy this node to the appropriate bit of the output.
          return CopyNodeToOutput(output_arg, output_offset, node);
        }

        // Otherwise, keep drilling down. Note that we iterate over bits in
        // "reverse" order, to match XLS' internal big-endian bit ordering (NOT
        // BYTE ORDERING) to the currently assumed little-endian bit ordering of
        // the host.
        for (int i = 0; i < bit_count; i++) {
          XLS_ASSIGN_OR_RETURN(
              std::string output,
              CollectNodeValue(node->operand(i), output_arg,
                               output_offset + (bit_count - i - 1)));
          absl::StrAppend(&outputs, output);
        }
        break;
      }
      case xls::TypeKind::kArray: {
        const xls::ArrayType* array_type = type->AsArrayOrDie();
        int64_t stride = array_type->element_type()->GetFlatBitCount();
        for (int i = 0; i < array_type->size(); i++) {
          XLS_ASSIGN_OR_RETURN(auto sub_output,
                               CollectNodeValue(node->operand(i), output_arg,
                                                output_offset + i * stride));
          absl::StrAppend(&outputs, sub_output);
        }
        break;
      }
      case xls::TypeKind::kTuple: {
        const xls::TupleType* tuple_type = type->AsTupleOrDie();
        int64_t sub_offset = 0;
        for (int i = 0; i < tuple_type->size(); i++) {
          XLS_ASSIGN_OR_RETURN(std::string sub_output,
                               CollectNodeValue(node->operand(i), output_arg,
                                                output_offset + sub_offset));
          sub_offset += node->operand(i)->GetType()->GetFlatBitCount();
          absl::StrAppend(&outputs, sub_output);
        }
        break;
      }
      default:
        return absl::InvalidArgumentError(
            absl::StrCat("Unsupported type kind: ", type->kind()));
    }
    return outputs;
  }

  static absl::StatusOr<std::string> TranslateNodes(
      const xls::Function* function) {
    std::string res;
    for (xls::Node* node :
         xls::TopoSort(const_cast<xls::Function*>(function))) {
      if (node->op() == xls::Op::kArray || node->op() == xls::Op::kArrayIndex ||
          node->op() == xls::Op::kConcat || node->op() == xls::Op::kParam ||
          node->op() == xls::Op::kShrl || node->op() == xls::Op::kTuple ||
          node->op() == xls::Op::kTupleIndex) {
        // Do nothing as HandleBitSlice and CollectNodeValue walk up the xls
        // tree to find the right bits to extract.
        continue;
      }

      absl::StrAppend(&res, InitializeNode(node));

      if (node->Is<xls::BitSlice>()) {
        XLS_ASSIGN_OR_RETURN(const std::string operation,
                             HandleBitSlice(node->As<xls::BitSlice>()));
        absl::StrAppend(&res, operation);
      } else {
        XLS_ASSIGN_OR_RETURN(const std::string operation, Execute(node));
        absl::StrAppend(&res, operation);
      }
    }
    return res;
  }
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_ABSTRACT_XLS_TRANSPILER_H_
