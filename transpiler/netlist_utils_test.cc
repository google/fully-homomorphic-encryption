// require('telescope').load_extension('projects') Copyright 2021 Google LLC
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
#include "transpiler/netlist_utils.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "xls/common/status/matchers.h"
#include "xls/common/status/status_macros.h"
#include "xls/netlist/netlist.h"

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

using ::testing::Eq;
using ::testing::StrEq;
using ::xls::netlist::rtl::AbstractNetDef;
using ::xls::netlist::rtl::NetDeclKind;
using ::xls::status_testing::IsOkAndHolds;
using ::xls::status_testing::StatusIs;

class TestTemplates : public CodegenTemplates {
 public:
  std::string ConstantCiphertext(int value) const override {
    return absl::StrFormat("my_lib.constant(%s)", value > 0 ? "true" : "false");
  }

  std::string PriorGateOutputReference(absl::string_view ref) const override {
    return absl::StrFormat("temp_nodes[%s]", ref);
  }

  std::string InputOrOutputReference(absl::string_view ref) const override {
    return std::string(ref);
  }
};

TEST(NetRefIdToNumericIdTest, Convert7) {
  EXPECT_THAT(NetRefIdToNumericId("_7_"), IsOkAndHolds(StrEq("7")));
}

TEST(NetRefIdToNumericIdTest, FailConvertNonInt) {
  EXPECT_THAT(NetRefIdToNumericId("_wat_"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(NetRefStemTest, ConvertIndexedNetRef) {
  EXPECT_EQ(NetRefStem("output[9]"), "output");
}

TEST(NetRefStemTest, ConvertNonIndexedNetRef) {
  EXPECT_EQ(NetRefStem("input"), "input");
}

TEST(ConstantToValueTest, Convert7) {
  EXPECT_THAT(ConstantToValue("<constant_7>"), IsOkAndHolds(Eq(7)));
}

TEST(ConstantToValueTest, Convert0) {
  EXPECT_THAT(ConstantToValue("<constant_0>"), IsOkAndHolds(Eq(0)));
}

TEST(ConstantToValueTest, FailConvertNonInt) {
  EXPECT_THAT(NetRefIdToNumericId("<constant_wat>"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ConstantToValueTest, FailConvertImproperlyStructured) {
  EXPECT_THAT(NetRefIdToNumericId("constant_7"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ResolveNetRefNameTest, convertWire) {
  TestTemplates templates;
  AbstractNetDef<bool> netDef("_17_", NetDeclKind::kWire);
  EXPECT_THAT(ResolveNetRefName(&netDef, templates),
              IsOkAndHolds(StrEq("temp_nodes[17]")));
}

TEST(ResolveNetRefNameTest, convertWireWithDifferentTemplate) {
  class TestTemplates2 : public CodegenTemplates {
   public:
    std::string ConstantCiphertext(int value) const override { return "wat"; }

    std::string InputOrOutputReference(absl::string_view ref) const override {
      return "wat";
    }
    std::string PriorGateOutputReference(absl::string_view ref) const override {
      return absl::StrFormat("wat[%s]", ref);
    }
  };
  TestTemplates2 templates;

  AbstractNetDef<bool> netDef("_17_", NetDeclKind::kWire);
  EXPECT_THAT(ResolveNetRefName(&netDef, templates),
              IsOkAndHolds(StrEq("wat[17]")));
}

TEST(ResolveNetRefNameTest, convertInout) {
  TestTemplates templates;
  AbstractNetDef<bool> netDef("foo[7]", NetDeclKind::kInput);
  EXPECT_THAT(ResolveNetRefName(&netDef, templates),
              IsOkAndHolds(StrEq("foo[7]")));
}

}  // namespace
}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
