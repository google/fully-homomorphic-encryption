#include "transpiler/jaxite/jaxite_xls_transpiler.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/jaxite/testing.h"
#include "xls/common/status/matchers.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/public/function_builder.h"
#include "xls/public/ir.h"
#include "xls/public/ir_parser.h"

namespace fhe {
namespace jaxite {
namespace transpiler {
namespace {

using xls::Function;
using xls::Package;
using ::xls::status_testing::StatusIs;

constexpr char kTestFunction[] = "test_fn";
constexpr char kTestPackage[] = "test_package";

absl::StatusOr<Function*> ParseAndGetFunction(Package* package,
                                              std::string_view program) {
  return xls::ParseFunctionIntoPackage(program, package);
}

// output element (e.g. a return value or an in/out parameter). For example,
// for an 8-bit output, this will return a Concat op which takes 8 literals as
// operands.
xls::BValue CreateElement(xls::FunctionBuilder* builder, int64_t bit_width,
                          absl::string_view name = "") {
  std::vector<xls::BValue> elements;
  elements.reserve(bit_width);
  for (int i = 0; i < bit_width; i++) {
    elements.push_back(builder->Literal(xls::UBits(1, 1)));
  }

  return builder->Concat(elements, xls::SourceInfo(), name);
}

// Tests that Jaxite returns an empty string ("") for any function and
// package. Jaxite targets a Python cryptographic backend, so there is no
// header file generated.
TEST(JaxiteXlsTranspilerLibTest, TranslateHeaderReturnsEmpty) {
  Package package(kTestPackage);
  XLS_ASSERT_OK_AND_ASSIGN(Function * function,
                           ParseAndGetFunction(&package, R"(
  fn test_fn() -> bits[32] {
    ret output: bits[32] = literal(value=2)
  }
  )"));
  xlscc_metadata::MetadataOutput metadata = CreateFunctionMetadata({}, true);

  XLS_ASSERT_OK_AND_ASSIGN(
      std::string actual, JaxiteXlsTranspiler::TranslateHeader(
                              function, metadata, /* header_path= */ "test.h"));
  EXPECT_EQ(actual, "");
}

TEST(JaxiteXlsTranspilerLibTest,
     TranslateFunctionSignatureWithNoParamWithReturnTypeSucceeds) {
  xls::Package package(kTestPackage);
  static constexpr absl::string_view no_param_fn = R"(
  fn test_fn() -> bits[32] {
    ret output: bits[32] = literal(value=2)
  }
  )";
  XLS_ASSERT_OK_AND_ASSIGN(Function * function,
                           ParseAndGetFunction(&package, no_param_fn));
  xlscc_metadata::MetadataOutput metadata = CreateFunctionMetadata({}, true);

  static constexpr absl::string_view expected =
      "def test_fn(result: List[types.LweCiphertext], sks: "
      "jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> None";
  XLS_ASSERT_OK_AND_ASSIGN(
      std::string actual,
      JaxiteXlsTranspiler::FunctionSignature(function, metadata));
  EXPECT_EQ(actual, expected);
}

TEST(JaxiteXlsTranspilerLibTest,
     TranslateFunctionSignatureWithParamAndVoidReturnTypeSucceeds) {
  xls::Package package(kTestPackage);
  XLS_ASSERT_OK_AND_ASSIGN(Function * function,
                           ParseAndGetFunction(&package, R"(
  fn test_fn(param: bits[32]) -> () {
    ret output: () = literal(value=())
  }
  )"));
  // Mark the top function as returning void with one parameter
  xlscc_metadata::MetadataOutput metadata =
      CreateFunctionMetadata({{"param", true}}, false);

  static constexpr absl::string_view expected =
      "def test_fn(param: List[types.LweCiphertext], sks: "
      "jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> None";
  XLS_ASSERT_OK_AND_ASSIGN(
      std::string actual,
      JaxiteXlsTranspiler::FunctionSignature(function, metadata));
  EXPECT_EQ(actual, expected);
}

TEST(JaxiteXlsTranspilerLibTest,
     TranslateFunctionSignatureWithReturnTypeAndNoParamSucceeds) {
  xls::Package package(kTestPackage);
  XLS_ASSERT_OK_AND_ASSIGN(Function * function,
                           ParseAndGetFunction(&package, R"(
  fn test_fn() -> bits[32] {
    ret output: bits[32] = literal(value=2)
  }
  )"));
  // Mark the top function as returning an int with no parameters
  xlscc_metadata::MetadataOutput metadata = CreateFunctionMetadata({}, true);

  static constexpr absl::string_view expected =
      "def test_fn(result: List[types.LweCiphertext], sks: "
      "jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> None";
  XLS_ASSERT_OK_AND_ASSIGN(
      std::string actual,
      JaxiteXlsTranspiler::FunctionSignature(function, metadata));
  EXPECT_EQ(actual, expected);
}

TEST(JaxiteXlsTranspilerLibTest, TranslateCopyNodeToOutputSucceeds) {
  xls::Package package(kTestPackage);
  xls::FunctionBuilder builder(kTestFunction, &package);
  xls::BValue single_bit = builder.Literal(xls::UBits(1, 1));

  static constexpr absl::string_view expected = "  result[0] = temp_nodes[1]\n";
  std::string actual = JaxiteXlsTranspiler::CopyNodeToOutput(
      "result", /* offset= */ 0, /* node= */ single_bit.node());
  EXPECT_EQ(actual, expected);
}

TEST(JaxiteXlsTranspilerLibTest, TranslateCopyParamToNodeSucceeds) {
  xls::Package package(kTestPackage);
  xls::FunctionBuilder builder(kTestFunction, &package);
  xls::BValue single_bit = builder.Literal(xls::UBits(1, 1));
  xls::BValue param =
      builder.Param("param", package.GetBitsType(kParamBitWidth));

  static constexpr absl::string_view expected = "  temp_nodes[1] = param[0]\n";
  std::string actual = JaxiteXlsTranspiler::CopyParamToNode(
      single_bit.node(), param.node(), /* offset= */ 0);
  EXPECT_EQ(actual, expected);
}

TEST(JaxiteXlsTranspilerLibTest, TranslateInitializeNodeReturnsEmpty) {
  xls::Package package(kTestPackage);
  xls::FunctionBuilder builder(kTestFunction, &package);
  xls::BValue single_bit = builder.Literal(xls::UBits(1, 1));

  std::string actual = JaxiteXlsTranspiler::InitializeNode(single_bit.node());
  EXPECT_EQ(actual, "");
}

TEST(JaxiteXlsTranspilerLibTest, TranslateExecuteWithAndGateSucceeds) {
  xls::Package package(kTestPackage);
  xls::FunctionBuilder builder(kTestFunction, &package);
  xls::BValue lhs = CreateElement(&builder, kParamBitWidth, "param_0");
  xls::BValue rhs = CreateElement(&builder, kParamBitWidth, "param_1");
  xls::BValue and_op =
      builder.And(lhs, rhs, xls::SourceInfo(), "param_0_and_param_1");

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           JaxiteXlsTranspiler::Execute(and_op.node()));

  static constexpr absl::string_view expected =
      "  temp_nodes[35] = jaxite_bool.and_(temp_nodes[17], temp_nodes[34], "
      "sks, params)\n\n";
  EXPECT_EQ(actual, expected);
}

TEST(JaxiteXlsTranspilerLibTest, TranslateExecuteWithOrGateSucceeds) {
  xls::Package package(kTestPackage);
  xls::FunctionBuilder builder(kTestFunction, &package);
  xls::BValue lhs = CreateElement(&builder, kParamBitWidth, "param_0");
  xls::BValue rhs = CreateElement(&builder, kParamBitWidth, "param_1");
  xls::BValue or_op =
      builder.Or(lhs, rhs, xls::SourceInfo(), "param_0_or_param_1");

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           JaxiteXlsTranspiler::Execute(or_op.node()));

  static constexpr absl::string_view expected =
      "  temp_nodes[35] = jaxite_bool.or_(temp_nodes[17], temp_nodes[34], "
      "sks, params)\n\n";
  EXPECT_EQ(actual, expected);
}

TEST(JaxiteXlsTranspilerLibTest, TranslateExecuteWithNotGateSucceeds) {
  xls::Package package(kTestPackage);
  xls::FunctionBuilder builder(kTestFunction, &package);
  xls::BValue param = CreateElement(&builder, kParamBitWidth, "param_0");
  xls::BValue not_op = builder.Not(param, xls::SourceInfo(), "not_param_0");

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           JaxiteXlsTranspiler::Execute(not_op.node()));

  static constexpr absl::string_view expected =
      "  temp_nodes[18] = jaxite_bool.not_(temp_nodes[17], params)\n\n";
  EXPECT_EQ(actual, expected);
}

TEST(JaxiteXlsTranspilerLibTest, TranslateExecuteWithLiteralGateSucceeds) {
  xls::Package package(kTestPackage);
  xls::FunctionBuilder builder(kTestFunction, &package);
  xls::BValue single_bit = builder.Literal(xls::UBits(1, 1));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           JaxiteXlsTranspiler::Execute(single_bit.node()));

  static constexpr absl::string_view expected =
      "  temp_nodes[1] = jaxite_bool.constant(True, params)\n\n";
  EXPECT_EQ(actual, expected);
}

TEST(JaxiteXlsTranspilerLibTest,
     TranslateExecuteWithUnusedLiteralReturnsEmpty) {
  xls::Package package(kTestPackage);
  xls::FunctionBuilder builder(kTestFunction, &package);

  // A multi-bit unused literal with no XLS "users".
  xls::BValue literal = builder.Literal(xls::UBits(3, 2));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           JaxiteXlsTranspiler::Execute(literal.node()));
  EXPECT_EQ(actual, "");
}

TEST(JaxiteXlsTranspilerLibTest, TranslateExecuteWithArrayIndexReturnsEmpty) {
  xls::Package package(kTestPackage);
  xls::FunctionBuilder builder(kTestFunction, &package);

  static constexpr int kBitWidth = 8;
  std::vector<xls::BValue> elements = {
      builder.Literal(xls::UBits(0, kBitWidth)),
      builder.Literal(xls::UBits(1, kBitWidth)),
      builder.Literal(xls::UBits(2, kBitWidth)),
  };
  xls::BValue array = builder.Array(elements, package.GetBitsType(kBitWidth));
  xls::BValue input_idx = builder.Literal(xls::UBits(2, kBitWidth));
  xls::BValue arr_index = builder.ArrayIndex(array, {input_idx});
  XLS_ASSERT_OK(builder.BuildWithReturnValue(arr_index));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           JaxiteXlsTranspiler::Execute(input_idx.node()));
  EXPECT_EQ(actual, "");
}

TEST(JaxiteXlsTranspilerLibTest,
     TranslateExecuteWithUnsupportedLiteralUserReturnsStatus) {
  xls::Package package(kTestPackage);
  xls::FunctionBuilder builder(kTestFunction, &package);

  static constexpr int kBitWidth = 8;
  std::vector<xls::BValue> elements = {
      builder.Literal(xls::UBits(0, kBitWidth)),
      builder.Literal(xls::UBits(1, kBitWidth)),
      builder.Literal(xls::UBits(2, kBitWidth)),
  };
  xls::BValue array = builder.Array(elements, package.GetBitsType(kBitWidth));
  xls::BValue start_idx = builder.Literal(xls::UBits(2, kBitWidth));
  xls::BValue arr_slice = builder.ArraySlice(array, start_idx, kBitWidth);
  XLS_ASSERT_OK(builder.BuildWithReturnValue(arr_slice));

  EXPECT_THAT(JaxiteXlsTranspiler::Execute(start_idx.node()),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(JaxiteXlsTranspilerLibTest,
     TranslateExecuteWithUnsupportedGateReturnsStatus) {
  xls::Package package(kTestPackage);
  xls::FunctionBuilder builder(kTestFunction, &package);
  xls::BValue lhs = CreateElement(&builder, kParamBitWidth, "param_0");
  xls::BValue rhs = CreateElement(&builder, kParamBitWidth, "param_1");
  xls::BValue eq_op =
      builder.Eq(lhs, rhs, xls::SourceInfo(), "param_0_eq_param_1");

  EXPECT_THAT(JaxiteXlsTranspiler::Execute(eq_op.node()),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(JaxiteXlsTranspilerLibTest, TranslatePreludeSucceeds) {
  xls::Package package(kTestPackage);

  XLS_ASSERT_OK_AND_ASSIGN(Function * function,
                           ParseAndGetFunction(&package, R"(
  fn test_fn() -> () {
    ret output: () = literal(value=())
  }
  )"));
  xlscc_metadata::MetadataOutput metadata = CreateFunctionMetadata({}, true);

  static constexpr absl::string_view expected = R"(from typing import Dict, List

from jaxite.jaxite_bool import jaxite_bool
from jaxite.jaxite_lib import types

def test_fn(result: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> None:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
)";
  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           JaxiteXlsTranspiler::Prelude(function, metadata));
  EXPECT_EQ(actual, expected);
}

TEST(JaxiteXlsTranspilerLibTest, TranslateConclusionReturnsEmpty) {
  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           JaxiteXlsTranspiler::Conclusion());
  EXPECT_EQ(actual, "");
}

}  // namespace
}  // namespace transpiler
}  // namespace jaxite
}  // namespace fhe
