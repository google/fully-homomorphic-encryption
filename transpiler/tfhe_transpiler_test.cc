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

#include <stdint.h>

#include <string>
#include <type_traits>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/strings/substitute.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/util/runfiles.h"
#include "transpiler/util/subprocess.h"
#include "transpiler/util/temp_file.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/status/matchers.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/public/function_builder.h"
#include "xls/public/ir.h"
#include "xls/public/ir_parser.h"

namespace fully_homomorphic_encryption::transpiler {
namespace {

using ::testing::UnorderedElementsAreArray;
using ::xls::status_testing::StatusIs;

absl::StatusOr<std::unique_ptr<xls::Package>> BooleanizeIr(xls::Package* p) {
  constexpr const char kBooleanifierPath[] = "xls/tools/booleanify_main";
  XLS_ASSIGN_OR_RETURN(TempFile temp_file, TempFile::Create());
  XLS_RETURN_IF_ERROR(xls::SetFileContents(temp_file.path(), p->DumpIr()));

  XLS_ASSIGN_OR_RETURN(std::string booleanifier_path,
                       GetRunfilePath(kBooleanifierPath, "com_google_xls"));
  XLS_ASSIGN_OR_RETURN(auto out_err,
                       InvokeSubprocess({
                           booleanifier_path,
                           absl::StrCat("--ir_path=", static_cast<std::string>(
                                                          temp_file.path())),
                           absl::StrCat("--top=", p->GetTop().value()->name()),
                       }));
  return xls::ParsePackage(std::get<0>(out_err), /*filename=*/std::nullopt);
}

// Creates an aggregate value containing individual bits representing an
// output. For example, for an 8-bit output, this will return a Concat op which
// takes 8 literals as operands.
xls::BValue CreateOutputElement(xls::FunctionBuilder* builder,
                                int64_t bit_width,
                                absl::optional<std::string> name) {
  std::vector<xls::BValue> elements;
  elements.reserve(bit_width);
  for (int i = 0; i < bit_width; i++) {
    elements.push_back(builder->Literal(xls::UBits(1, 1)));
  }

  return builder->Concat(elements, xls::SourceInfo(),
                         name.has_value() ? name.value() : "");
}

// Extremely basic "does anything work" test.
TEST(TfheIrTranspilerLibTest, CollectOutputValuesSmoke) {
  constexpr int kPureReturnWidth = 8;
  constexpr int kInOutWidth = 16;

  xls::Package package("test_package");
  xls::FunctionBuilder builder("smoke", &package);

  // One pure return, one in/out param.
  xls::BValue pure_return =
      CreateOutputElement(&builder, kPureReturnWidth, "pure_return");
  xls::BValue in_out_return =
      CreateOutputElement(&builder, kInOutWidth, "in_out_param");
  builder.Param("in_out_param", package.GetBitsType(kInOutWidth));
  xls::BValue in_out_tuple = builder.Tuple(absl::MakeSpan(&in_out_return, 1));
  xls::BValue final_tuple = builder.Tuple({pure_return, in_out_tuple});
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function,
                           builder.BuildWithReturnValue(final_tuple));

  // This type of testing is not ideal - it depends on knowing how the
  // IR internals assign indices to the internal nodes. This is unlikely to
  // change all that often, but it still may change.
  std::vector<std::string> expected_lines;
  for (int i = 0; i < kPureReturnWidth; i++) {
    expected_lines.push_back(
        absl::Substitute("  bootsCOPY(&result[$0], temp_nodes[$1], bk);",
                         kPureReturnWidth - i - 1, i + 1));
  }
  for (int i = 0; i < kInOutWidth; i++) {
    expected_lines.push_back(
        absl::Substitute("  bootsCOPY(&in_out_param[$0], temp_nodes[$1], bk);",
                         kInOutWidth - i - 1, kPureReturnWidth + 2 + i));
  }

  xlscc_metadata::MetadataOutput metadata;
  xlscc_metadata::FunctionParameter* param =
      metadata.mutable_top_func_proto()->add_params();
  param->set_name("in_out_param");
  param->set_is_reference(true);

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::CollectOutputs(function, metadata));

  EXPECT_THAT(absl::StrSplit(absl::StripSuffix(actual, "\n"), '\n'),
              UnorderedElementsAreArray(expected_lines));
}

// This test verifies that CollectOutputValues still works when there's no
// output at all - no formal, and no in/out.
TEST(TfheIrTranspilerLibTest, CollectOutputValues_NoOutput) {
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  std::vector<xls::Value> dummy;
  xls::BValue empty_tuple = builder.Literal(xls::Value::Tuple(dummy));
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function,
                           builder.BuildWithReturnValue(empty_tuple));

  // There'll be no output in this case. Let's just make sure we don't blow up.
  XLS_ASSERT_OK(TfheTranspiler::CollectOutputs(function, {}));
}

// This test verifies that CollectOutputValues still works when there's a pure
// output but no in/out.
TEST(TfheIrTranspilerLibTest, CollectOutputValues_OnlyPure) {
  constexpr int kPureReturnWidth = 64;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  xls::BValue pure_return =
      CreateOutputElement(&builder, kPureReturnWidth, "pure_return");
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function,
                           builder.BuildWithReturnValue(pure_return));
  std::vector<std::string> expected_lines;
  for (int i = 0; i < kPureReturnWidth; i++) {
    expected_lines.push_back(
        absl::Substitute("  bootsCOPY(&result[$0], temp_nodes[$1], bk);",
                         kPureReturnWidth - i - 1, i + 1));
  }

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::CollectOutputs(function, {}));
  EXPECT_THAT(absl::StrSplit(absl::StripSuffix(actual, "\n"), '\n'),
              UnorderedElementsAreArray(expected_lines));
}

// This test verifies that CollectOutputValues still works when there's only
// in/out params.
TEST(TfheIrTranspilerLibTest, CollectOutputValues_OnlyInOut) {
  constexpr int kInOutWidth = 64;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  xls::BValue in_out_return =
      CreateOutputElement(&builder, kInOutWidth, "in_out_return");
  xls::BValue in_out_tuple = builder.Tuple(absl::MakeSpan(&in_out_return, 1));
  xls::BValue final_tuple = builder.Tuple({in_out_tuple});
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function,
                           builder.BuildWithReturnValue(final_tuple));
  std::vector<std::string> expected_lines;
  for (int i = 0; i < kInOutWidth; i++) {
    expected_lines.push_back(
        absl::Substitute("  bootsCOPY(&result[$0], temp_nodes[$1], bk);",
                         kInOutWidth - i - 1, i + 1));
  }

  xlscc_metadata::MetadataOutput metadata;
  xlscc_metadata::FunctionParameter* param =
      metadata.mutable_top_func_proto()->add_params();
  param->set_name("in_out_param");

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::CollectOutputs(function, metadata));
  EXPECT_THAT(absl::StrSplit(absl::StripSuffix(actual, "\n"), '\n'),
              UnorderedElementsAreArray(expected_lines));
}

TEST(TfheIrTranspilerLibTest, CollectOutputValues_MultipleInOut) {
  constexpr int kPureWidth = 8;
  constexpr int kNumInOutParams = 3;
  constexpr int kInOutWidth = 8;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  std::vector<xls::BValue> params;
  params.push_back(CreateOutputElement(&builder, kPureWidth, "pure_return"));
  std::vector<std::string> in_out_names;
  for (int i = 0; i < kNumInOutParams; i++) {
    in_out_names.push_back(absl::StrCat("in_out_", i));
    params.push_back(
        CreateOutputElement(&builder, kInOutWidth, in_out_names.back()));
  }

  xls::BValue final_tuple = builder.Tuple(params);
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function,
                           builder.BuildWithReturnValue(final_tuple));
  int output_idx = 1;
  std::vector<std::string> expected_lines;
  for (int i = 0; i < kPureWidth; i++) {
    expected_lines.push_back(
        absl::Substitute("  bootsCOPY(&result[$0], temp_nodes[$1], bk);",
                         kPureWidth - i - 1, output_idx++));
  }
  for (int i = 0; i < kNumInOutParams; i++) {
    output_idx++;
    for (int j = 0; j < kInOutWidth; j++) {
      expected_lines.push_back(
          absl::Substitute("  bootsCOPY(&$0[$1], temp_nodes[$2], bk);",
                           in_out_names[i], kInOutWidth - j - 1, output_idx++));
    }
  }

  xlscc_metadata::MetadataOutput metadata;
  for (int i = 0; i < kNumInOutParams; i++) {
    xlscc_metadata::FunctionParameter* param =
        metadata.mutable_top_func_proto()->add_params();
    param->set_name(absl::StrCat("in_out_", i));
    param->set_is_reference(true);
  }

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::CollectOutputs(function, metadata));
  EXPECT_THAT(absl::StrSplit(absl::StripSuffix(actual, "\n"), '\n'),
              UnorderedElementsAreArray(expected_lines));
}

// This test verifies that CollectOutputValues can properly handle an
// array-valued output.
TEST(TfheIrTranspilerLibTest, CollectOutputValues_PureArray) {
  constexpr int kPureWidth = 8;
  constexpr int kPureElements = 4;

  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  std::vector<xls::BValue> elements;
  elements.reserve(kPureElements);
  for (int i = 0; i < kPureElements; i++) {
    elements.push_back(
        CreateOutputElement(&builder, kPureWidth, absl::StrCat("element_", i)));
  }
  xls::BValue pure_array =
      builder.Array(elements, package.GetBitsType(kPureWidth));
  std::vector<xls::BValue> in_out_elements;
  xls::BValue in_out_tuple = builder.Tuple(in_out_elements);
  xls::BValue final_tuple = builder.Tuple({pure_array, in_out_tuple});
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function,
                           builder.BuildWithReturnValue(final_tuple));
  std::vector<std::string> expected_lines;
  int temp_index = 0;
  for (int i = 0; i < kPureElements; i++) {
    temp_index++;
    for (int j = 0; j < kPureWidth; j++) {
      expected_lines.push_back(
          absl::Substitute("  bootsCOPY(&result[$0], temp_nodes[$1], bk);",
                           i * kPureWidth + kPureWidth - j - 1, temp_index++));
    }
  }

  xlscc_metadata::MetadataOutput metadata;
  xlscc_metadata::FunctionParameter* param =
      metadata.mutable_top_func_proto()->add_params();
  param->set_name("in_out_param");
  param->set_is_reference(true);
  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::CollectOutputs(function, metadata));

  EXPECT_THAT(absl::StrSplit(absl::StripSuffix(actual, "\n"), '\n'),
              UnorderedElementsAreArray(expected_lines));
}

// This test verifies that CollectOutputs correctly skips over const and
// non-reference params.
TEST(TfheIrTranspilerLibTest, CollectOutputValues_SkipsConstAndNonRefs) {
  constexpr int kArrayElements = 4;
  constexpr int kElementBits = 8;
  constexpr int kPureWidth = 8;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  std::vector<xls::BValue> params;
  params.push_back(CreateOutputElement(&builder, kPureWidth, "pure_return"));

  xls::Type* i8_type = package.GetBitsType(kElementBits);
  builder.Param("const_ref", package.GetArrayType(4, i8_type));
  builder.Param("non_const_ref", package.GetArrayType(4, i8_type));
  builder.Param("non_const_non_ref", i8_type);
  builder.Param("const_non_ref", i8_type);

  // Set every other bit in the output. This is easier than manually exploding
  // then concatting the input array (which is necessary to make this
  // "booleanized" and thus not explode).
  xls::BValue bit_zero = builder.Literal(xls::Value(xls::UBits(0, 1)));
  xls::BValue bit_one = builder.Literal(xls::Value(xls::UBits(1, 1)));
  std::vector<xls::BValue> new_elements;
  for (int i = 0; i < kArrayElements; i++) {
    std::vector<xls::BValue> element_bits;
    for (int j = 0; j < kElementBits; j++) {
      element_bits.push_back(j & 1 ? bit_one : bit_zero);
    }
    new_elements.push_back(builder.Concat(element_bits));
  }

  std::vector<xls::BValue> outputs;
  outputs.push_back(CreateOutputElement(&builder, kPureWidth, "pure_return"));
  outputs.push_back(builder.Array(new_elements, i8_type));
  xls::BValue final_tuple = builder.Tuple(outputs);
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function,
                           builder.BuildWithReturnValue(final_tuple));

  xlscc_metadata::MetadataOutput metadata;
  xlscc_metadata::FunctionParameter* param =
      metadata.mutable_top_func_proto()->add_params();
  param->set_name("const_ref");
  param->set_is_const(true);
  param->set_is_reference(true);
  param = metadata.mutable_top_func_proto()->add_params();
  param->set_name("non_const_ref");
  param->set_is_const(false);
  param->set_is_reference(true);
  param = metadata.mutable_top_func_proto()->add_params();
  param->set_name("non_const_non_ref");
  param->set_is_const(false);
  param->set_is_reference(false);
  param = metadata.mutable_top_func_proto()->add_params();
  param->set_name("const_non_ref");
  param->set_is_const(true);
  param->set_is_reference(false);

  // Finally, create the expected output.
  int output_idx = 20;
  std::vector<std::string> expected_lines;
  for (int i = 0; i < kPureWidth; i++) {
    expected_lines.push_back(
        absl::Substitute("  bootsCOPY(&result[$0], temp_nodes[$1], bk);",
                         kPureWidth - i - 1, output_idx++));
  }

  // Node 14 is bit_zero, and 15 is bit one from above.
  int current_bit = kArrayElements * kElementBits - 1;
  for (int i = 0; i < kArrayElements; i++) {
    for (int j = 0; j < kElementBits; j++) {
      expected_lines.push_back(absl::Substitute(
          "  bootsCOPY(&non_const_ref[$0], temp_nodes[$1], bk);", current_bit--,
          j & 1 ? 15 : 14));
    }
  }

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::CollectOutputs(function, metadata));
  EXPECT_THAT(absl::StrSplit(absl::StripSuffix(actual, "\n"), '\n'),
              UnorderedElementsAreArray(expected_lines));
}

// This test verifies that CollectOutputValues can properly handle an
// 2D array-valued output.
TEST(TfheIrTranspilerLibTest, CollectOutputValues_Pure2DArray) {
  constexpr int kPureWidth = 8;
  constexpr int kTopElements = 4;
  constexpr int kSubElements = 4;

  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  xls::Type* leaf_type = package.GetBitsType(kPureWidth);
  xls::Type* subarray_type = package.GetArrayType(kSubElements, leaf_type);

  std::vector<xls::BValue> elements;
  for (int i = 0; i < kTopElements; i++) {
    std::vector<xls::BValue> sub_elements;
    for (int j = 0; j < kSubElements; j++) {
      sub_elements.push_back(CreateOutputElement(
          &builder, kPureWidth, absl::StrCat("element_", i, "_", j)));
    }
    elements.push_back(builder.Array(sub_elements, leaf_type));
  }

  xls::BValue pure_array = builder.Array(elements, subarray_type);
  std::vector<xls::BValue> in_out_elements;
  xls::BValue in_out_tuple = builder.Tuple(in_out_elements);
  xls::BValue final_tuple = builder.Tuple({pure_array, in_out_tuple});
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function,
                           builder.BuildWithReturnValue(final_tuple));

  std::vector<std::string> expected_lines;
  int temp_index = -1;
  for (int top_idx = 0; top_idx < kTopElements; top_idx++) {
    temp_index++;
    for (int sub_idx = 0; sub_idx < kSubElements; sub_idx++) {
      temp_index++;
      for (int bit_idx = 0; bit_idx < kPureWidth; bit_idx++) {
        expected_lines.push_back(
            absl::Substitute("  bootsCOPY(&result[$0], temp_nodes[$1], bk);",
                             top_idx * subarray_type->GetFlatBitCount() +
                                 sub_idx * leaf_type->GetFlatBitCount() +
                                 kPureWidth - bit_idx - 1,
                             temp_index++));
      }
    }
  }

  xlscc_metadata::MetadataOutput metadata;
  xlscc_metadata::FunctionParameter* param =
      metadata.mutable_top_func_proto()->add_params();
  param->set_name("in_out_param");
  param->set_is_reference(true);
  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::CollectOutputs(function, metadata));

  EXPECT_THAT(absl::StrSplit(absl::StripSuffix(actual, "\n"), '\n'),
              UnorderedElementsAreArray(expected_lines));
}

// This test verifies that `TfheTranspiler::FunctionSignature` produces the
// correct value when provided a function with output but no in/out parameters.
TEST(TfheIrTranspilerLibTest, FunctionSignature_OnlyPure) {
  constexpr int kPureReturnWidth = 64;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  xls::BValue pure_return =
      CreateOutputElement(&builder, kPureReturnWidth, "pure_return");

  xlscc_metadata::MetadataOutput metadata;
  xlscc_metadata::IntType* int_return_type = metadata.mutable_top_func_proto()
                                                 ->mutable_return_type()
                                                 ->mutable_as_int();
  metadata.mutable_top_func_proto()->mutable_name()->set_name("test_fn");
  int_return_type->set_width(kPureReturnWidth);
  int_return_type->set_is_signed(true);

  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function,
                           builder.BuildWithReturnValue(pure_return));
  XLS_ASSERT_OK_AND_ASSIGN(
      std::string function_signature,
      TfheTranspiler::FunctionSignature(function, metadata));

  static constexpr absl::string_view expected_signature =
      R"(absl::Status test_fn_UNSAFE(absl::Span<LweSample> result, const TFheGateBootstrappingCloudKeySet* bk))";
  EXPECT_EQ(function_signature, expected_signature);
}

// This test verifies that `TfheTranspiler::Prelude` produces the correct value
// when provided a function with output but no in/out parameters.
TEST(TfheIrTranspilerLibTest, Prelude_OnlyPure) {
  constexpr int kPureReturnWidth = 64;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  xls::BValue pure_return =
      CreateOutputElement(&builder, kPureReturnWidth, "pure_return");

  xlscc_metadata::MetadataOutput metadata;
  xlscc_metadata::IntType* int_return_type = metadata.mutable_top_func_proto()
                                                 ->mutable_return_type()
                                                 ->mutable_as_int();
  metadata.mutable_top_func_proto()->mutable_name()->set_name("test_fn");
  int_return_type->set_width(kPureReturnWidth);
  int_return_type->set_is_signed(true);

  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function,
                           builder.BuildWithReturnValue(pure_return));
  XLS_ASSERT_OK_AND_ASSIGN(std::string prelude,
                           TfheTranspiler::Prelude(function, metadata));

  static constexpr absl::string_view expected_prelude =
      R"(#include <unordered_map>

#include "absl/status/status.h"
#include "absl/types/span.h"
#include "transpiler/common_runner.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"

static StructReverseEncodeOrderSetter ORDER;

absl::Status test_fn_UNSAFE(absl::Span<LweSample> result, const TFheGateBootstrappingCloudKeySet* bk) {
  std::unordered_map<int, LweSample*> temp_nodes;

)";

  EXPECT_EQ(prelude, expected_prelude);
}

// This test verifies that `TfheTranspiler::Conclusion` produces the correct
// value.
TEST(TfheIrTranspilerLibTest, Conclusion) {
  XLS_ASSERT_OK_AND_ASSIGN(std::string conclusion,
                           TfheTranspiler::Conclusion());

  static constexpr absl::string_view expected_conclusion =
      R"(  for (auto pair : temp_nodes) {
    delete_gate_bootstrapping_ciphertext(pair.second);
  }
  return absl::OkStatus();
}
)";
  EXPECT_EQ(conclusion, expected_conclusion);
}

// This test verifies that TranslateHeader works when there's no parameters.
TEST(TfheIrTranspilerLibTest, TranslateHeader_NoParam) {
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  std::vector<xls::Value> dummy;
  xls::BValue empty_tuple = builder.Literal(xls::Value::Tuple(dummy));
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function,
                           builder.BuildWithReturnValue(empty_tuple));

  // Mark the top function as returning void with no parameters.
  xlscc_metadata::MetadataOutput metadata;
  metadata.mutable_top_func_proto()->mutable_return_type()->mutable_as_void();
  metadata.mutable_top_func_proto()->mutable_name()->set_name("test_fn");

  static constexpr absl::string_view expected_header =
      R"(#ifndef A_B_C_TEST_H
#define A_B_C_TEST_H

#include "test.types.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "transpiler/data/tfhe_data.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"

absl::Status test_fn_UNSAFE(const TFheGateBootstrappingCloudKeySet* bk);

absl::Status test_fn(const TFheGateBootstrappingCloudKeySet* bk) {
  return test_fn_UNSAFE(bk);
}
#endif  // A_B_C_TEST_H
)";
  XLS_ASSERT_OK_AND_ASSIGN(
      std::string actual,
      TfheTranspiler::TranslateHeader(function, metadata, "a/b/c/test.h",
                                      "test.types.h",
                                      /*skip_scheme_data_deps=*/false, {}));
  EXPECT_EQ(actual, expected_header);
}

// This test verifies that TranslateHeader works when there's one parameter.
TEST(TfheIrTranspilerLibTest, TranslateHeader_Param) {
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);
  xls::BitsType* value_type = package.GetBitsType(32);
  builder.Param("param", value_type);
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function, builder.Build());

  // Mark the top function as returning void with one parameter.
  xlscc_metadata::MetadataOutput metadata;
  metadata.mutable_top_func_proto()->mutable_return_type()->mutable_as_void();
  metadata.mutable_top_func_proto()->mutable_name()->set_name("test_fn");
  xlscc_metadata::FunctionParameter* param =
      metadata.mutable_top_func_proto()->add_params();
  param->set_name("param");
  xlscc_metadata::IntType* param_int_type =
      param->mutable_type()->mutable_as_int();
  param_int_type->set_is_signed(true);
  param_int_type->set_width(16);

  static constexpr absl::string_view expected_header =
      R"(#ifndef TEST_H
#define TEST_H

#include "test.types.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "transpiler/data/tfhe_data.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"

absl::Status test_fn_UNSAFE(absl::Span<const LweSample> param, const TFheGateBootstrappingCloudKeySet* bk);

absl::Status test_fn(const TfheRef<signed short> param,
 const TFheGateBootstrappingCloudKeySet* bk) {
  return test_fn_UNSAFE(param.get(), bk);
}
#endif  // TEST_H
)";
  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::TranslateHeader(
                               function, metadata, "test.h", "test.types.h",
                               /*skip_scheme_data_deps=*/false, {}));
  EXPECT_EQ(actual, expected_header);
}

// This test verifies that TranslateHeader works when there are multiple
// parameters.
TEST(TfheIrTranspilerLibTest, TranslateHeader_MultipleParams) {
  constexpr int kParamNum = 5;
  xls::Package package("test_package");
  xlscc_metadata::MetadataOutput metadata;
  // Mark the top function as returning void.
  metadata.mutable_top_func_proto()->mutable_return_type()->mutable_as_void();
  metadata.mutable_top_func_proto()->mutable_name()->set_name("test_fn");

  xls::FunctionBuilder builder("test_fn", &package);
  xls::BitsType* value_type = package.GetBitsType(32);
  for (int param_index = 0; param_index < kParamNum; param_index++) {
    const std::string param_name = absl::StrCat("param_", param_index);
    builder.Param(param_name, value_type);
    xlscc_metadata::FunctionParameter* param =
        metadata.mutable_top_func_proto()->add_params();
    param->set_name(param_name);
    param->mutable_type()->mutable_as_bool();
  }
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function, builder.Build());

  static constexpr absl::string_view expected_header_template =
      R"(#ifndef TEST_H
#define TEST_H

#include "test.types.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "transpiler/data/tfhe_data.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"

absl::Status test_fn_UNSAFE($0, const TFheGateBootstrappingCloudKeySet* bk);

absl::Status test_fn(const TfheRef<bool> param_0, const TfheRef<bool> param_1, const TfheRef<bool> param_2, const TfheRef<bool> param_3, const TfheRef<bool> param_4,
 const TFheGateBootstrappingCloudKeySet* bk) {
  return test_fn_UNSAFE(param_0.get(), param_1.get(), param_2.get(), param_3.get(), param_4.get(), bk);
}
#endif  // TEST_H
)";
  std::vector<std::string> expected_params;
  for (int param_index = 0; param_index < kParamNum; param_index++) {
    expected_params.push_back(
        absl::StrCat("absl::Span<const LweSample> param_", param_index));
  }
  std::string expected_header = absl::Substitute(
      expected_header_template, absl::StrJoin(expected_params, ", "));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::TranslateHeader(
                               function, metadata, "test.h", "test.types.h",
                               /*skip_scheme_data_deps=*/false, {}));
  EXPECT_EQ(actual, expected_header);
}

TEST(TfheIrTranspilerLibTest, InitializeNode) {
  constexpr int kInOutWidth = 8;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);
  xls::BitsType* value_type = package.GetBitsType(kInOutWidth);
  xls::BValue param = builder.Param(absl::StrCat("param_", 0), value_type);

  std::string actual = TfheTranspiler::InitializeNode(param.node());
  EXPECT_EQ(
      actual,
      absl::Substitute(
          "  temp_nodes[$0] = new_gate_bootstrapping_ciphertext(bk->params);\n",
          param.node()->id()));
}

TEST(TfheIrTranspilerLibTest, Execute_AndOp) {
  constexpr int kInOutWidth = 8;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);
  xls::BValue lhs =
      CreateOutputElement(&builder, kInOutWidth, absl::StrCat("param_", 0));
  xls::BValue rhs =
      CreateOutputElement(&builder, kInOutWidth, absl::StrCat("param_", 1));
  xls::BValue and_op =
      builder.And(lhs, rhs, xls::SourceInfo(), "param_0_and_param_1");

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::Execute(and_op.node()));
  EXPECT_EQ(
      actual,
      absl::Substitute(
          "  bootsAND(temp_nodes[$0], temp_nodes[$1], temp_nodes[$2], bk);\n\n",
          and_op.node()->id(), lhs.node()->id(), rhs.node()->id()));
}

TEST(TfheIrTranspilerLibTest, Execute_OrOp) {
  constexpr int kInOutWidth = 32;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);
  xls::BValue lhs =
      CreateOutputElement(&builder, kInOutWidth, absl::StrCat("param_", 0));
  xls::BValue rhs =
      CreateOutputElement(&builder, kInOutWidth, absl::StrCat("param_", 1));
  xls::BValue or_op =
      builder.Or(lhs, rhs, xls::SourceInfo(), "param_0_or_param_1");

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::Execute(or_op.node()));
  EXPECT_EQ(
      actual,
      absl::Substitute(
          "  bootsOR(temp_nodes[$0], temp_nodes[$1], temp_nodes[$2], bk);\n\n",
          or_op.node()->id(), lhs.node()->id(), rhs.node()->id()));
}

TEST(TfheIrTranspilerLibTest, Execute_NotOp) {
  constexpr int kInOutWidth = 64;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);
  xls::BValue param =
      CreateOutputElement(&builder, kInOutWidth, absl::StrCat("param_", 0));
  xls::BValue not_op = builder.Not(param, xls::SourceInfo(), "not_param_0");

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::Execute(not_op.node()));
  EXPECT_EQ(actual, absl::Substitute(
                        "  bootsNOT(temp_nodes[$0], temp_nodes[$1], bk);\n\n",
                        not_op.node()->id(), param.node()->id()));
}

TEST(TfheIrTranspilerLibTest, Execute_InvalidOp) {
  constexpr int kInOutWidth = 16;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);
  xls::BValue lhs =
      CreateOutputElement(&builder, kInOutWidth, absl::StrCat("param_", 0));
  xls::BValue rhs =
      CreateOutputElement(&builder, kInOutWidth, absl::StrCat("param_", 1));
  xls::BValue eq_op =
      builder.Eq(lhs, rhs, xls::SourceInfo(), "param_0_eq_param_1");

  EXPECT_THAT(TfheTranspiler::Execute(eq_op.node()),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(TfheIrTranspilerLibTest, Execute_LiteralOne) {
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  // Single bit with value 1
  xls::BValue literal = builder.Literal(xls::UBits(1, 1));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::Execute(literal.node()));
  EXPECT_EQ(actual,
            absl::Substitute("  bootsCONSTANT(temp_nodes[$0], 1, bk);\n\n",
                             literal.node()->id()));
}

TEST(TfheIrTranspilerLibTest, Execute_LiteralZero) {
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  // Single bit with value 0
  xls::BValue literal = builder.Literal(xls::UBits(0, 1));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           TfheTranspiler::Execute(literal.node()));
  EXPECT_EQ(actual,
            absl::Substitute("  bootsCONSTANT(temp_nodes[$0], 0, bk);\n\n",
                             literal.node()->id()));
}

TEST(TfheIrTranspilerLibTest, Execute_UnsupportedLiteral) {
  constexpr int kInOutWidth = 16;
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);
  xls::BValue param =
      CreateOutputElement(&builder, kInOutWidth, absl::StrCat("param_", 0));

  EXPECT_THAT(TfheTranspiler::Execute(param.node()),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

// This test verifies that CollectOutputValues can properly handle a
// two-dimensional array_index.
// Takes in a bits[2][3][4][5] and outputs a bits[2].
#if 0
TEST(TfheIrTranspilerLibTest, Handles2dInputArray) {
  xls::Package package("test_package");
  xls::FunctionBuilder builder("test_fn", &package);

  xls::Type* inner_type = package.GetArrayType(3, package.GetBitsType(2));
  xls::Type* middle_type = package.GetArrayType(4, inner_type);
  xls::Type* outer_type = package.GetArrayType(5, middle_type);
  xls::BValue input_a = builder.Param("a", outer_type);
  std::vector<xls::BValue> indices = {
      builder.Literal(xls::UBits(1, 8)),
      builder.Literal(xls::UBits(2, 8)),
      builder.Literal(xls::UBits(3, 8)),
  };
  xls::BValue input_idx = builder.Array(indices, package.GetBitsType(8));
  xls::BValue idx_0 =
      builder.ArrayIndex(input_idx, {builder.Literal(xls::UBits(0, 8))});
  xls::BValue idx_1 =
      builder.ArrayIndex(input_idx, {builder.Literal(xls::UBits(1, 8))});
  xls::BValue idx_2 =
      builder.ArrayIndex(input_idx, {builder.Literal(xls::UBits(2, 8))});
  xls::BValue array_idx = builder.ArrayIndex(input_a, {idx_0, idx_1, idx_2});
  XLS_ASSERT_OK(builder.BuildWithReturnValue(array_idx).status());

  xlscc_metadata::MetadataOutput metadata;
  xlscc_metadata::FunctionParameter* param =
      metadata.mutable_top_func_proto()->add_params();
  param->set_name("a");
  param->set_is_reference(true);
  param->set_is_const(true);
  param = metadata.mutable_top_func_proto()->add_params();
  param->set_name("idx");
  param->set_is_reference(true);
  param->set_is_const(true);
  XLS_ASSERT_OK_AND_ASSIGN(auto bool_package, BooleanizeIr(&package));
  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                       TfheTranspiler::Translate(
                           bool_package->EntryFunction().value(), metadata));

  EXPECT_TRUE(absl::StrContains(actual, "&result[0]"));
  EXPECT_TRUE(absl::StrContains(actual, "&result[1]"));
  EXPECT_FALSE(absl::StrContains(actual, "&result[2]"));
}
#endif

}  // namespace
}  // namespace fully_homomorphic_encryption::transpiler
