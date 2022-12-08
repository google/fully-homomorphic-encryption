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

#include "transpiler/cleartext_transpiler.h"

#include <string>
#include <type_traits>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "xls/common/status/matchers.h"
#include "xls/common/status/status_macros.h"
#include "xls/public/function_builder.h"
#include "xls/public/ir.h"

namespace fully_homomorphic_encryption::transpiler {
namespace {

// This test verifies that TranslateHeader works when there's no parameters.
TEST(CleartextTranspilerLibTest, TranslateHeader_NoParam) {
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
      R"(#ifndef TEST_H
#define TEST_H

#include "test.types.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "transpiler/data/cleartext_data.h"

absl::Status test_fn_UNSAFE();
absl::Status test_fn() {
  return test_fn_UNSAFE();
}
#endif  // TEST_H
)";
  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           CleartextTranspiler::TranslateHeader(
                               function, metadata, "test.h", "test.types.h",
                               /*skip_scheme_data_deps=*/false, {}));
  EXPECT_EQ(actual, expected_header);
}

// This test verifies that TranslateHeader works when there's one parameter.
TEST(CleartextTranspilerLibTest, TranslateHeader_Param) {
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
  param->mutable_type()->mutable_as_int()->set_width(32);

  static constexpr absl::string_view expected_header =
      R"(#ifndef TEST_H
#define TEST_H

#include "test.types.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "transpiler/data/cleartext_data.h"

absl::Status test_fn_UNSAFE(absl::Span<const bool> param);
absl::Status test_fn(const EncodedRef<unsigned int> param) {
  return test_fn_UNSAFE(param.get());
}
#endif  // TEST_H
)";
  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           CleartextTranspiler::TranslateHeader(
                               function, metadata, "test.h", "test.types.h",
                               /*skip_scheme_data_deps=*/false, {}));
  EXPECT_EQ(actual, expected_header);
}

// This test verifies that TranslateHeader works when there are multiple
// parameters.
TEST(CleartextTranspilerLibTest, TranslateHeader_MultipleParams) {
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
    param->mutable_type()->mutable_as_int()->set_width(32);
  }
  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * function, builder.Build());

  static constexpr absl::string_view expected_header_template =
      R"(#ifndef TEST_H
#define TEST_H

#include "test.types.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "transpiler/data/cleartext_data.h"

absl::Status test_fn_UNSAFE($0);
absl::Status test_fn($1) {
  return test_fn_UNSAFE($2);
}
#endif  // TEST_H
)";
  std::vector<std::string> param_names;
  std::vector<std::string> expected_params;
  std::vector<std::string> expected_overloaded_params;
  for (int param_index = 0; param_index < kParamNum; param_index++) {
    param_names.push_back(absl::StrCat("param_", param_index, ".get()"));
    expected_params.push_back(
        absl::StrCat("absl::Span<const bool> param_", param_index));
    expected_overloaded_params.push_back(
        absl::StrCat("const EncodedRef<unsigned int> param_", param_index));
  }
  std::string expected_header = absl::Substitute(
      expected_header_template, absl::StrJoin(expected_params, ", "),
      absl::StrJoin(expected_overloaded_params, ", "),
      absl::StrJoin(param_names, ", "));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           CleartextTranspiler::TranslateHeader(
                               function, metadata, "test.h", "test.types.h",
                               /*skip_scheme_data_deps=*/false, {}));
  EXPECT_EQ(actual, expected_header);
}

}  // namespace

}  // namespace fully_homomorphic_encryption::transpiler
