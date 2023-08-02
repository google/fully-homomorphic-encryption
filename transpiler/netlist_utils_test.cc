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
#include "xls/netlist/cell_library.h"
#include "xls/netlist/netlist.h"

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::StrEq;
using ::testing::UnorderedElementsAre;
using ::xls::netlist::AbstractCellLibrary;
using ::xls::netlist::rtl::AbstractModule;
using ::xls::netlist::rtl::AbstractNetDef;
using ::xls::netlist::rtl::AbstractNetlist;
using ::xls::netlist::rtl::NetDeclKind;
using ::xls::status_testing::IsOk;
using ::xls::status_testing::IsOkAndHolds;
using ::xls::status_testing::StatusIs;

constexpr absl::string_view kCells = R"lib(
library(testlib) {
  cell(and2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A * B)";}}
  cell(or2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A + B)";}}
}
)lib";

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

absl::StatusOr<std::vector<std::string>> run_toposort(
    absl::string_view netlist) {
  XLS_ASSIGN_OR_RETURN(AbstractCellLibrary<bool> cell_library,
                       ParseCellLibrary(std::string(kCells)));
  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<AbstractNetlist<bool>> parsed_netlist_ptr,
      ParseNetlist(cell_library, netlist));
  const AbstractModule<bool>& module = *parsed_netlist_ptr->modules().front();

  return TopoSortedCellNames(module);
}

absl::StatusOr<std::vector<std::vector<std::string>>> run_levelsort(
    absl::string_view netlist) {
  XLS_ASSIGN_OR_RETURN(AbstractCellLibrary<bool> cell_library,
                       ParseCellLibrary(std::string(kCells)));
  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<AbstractNetlist<bool>> parsed_netlist_ptr,
      ParseNetlist(cell_library, netlist));
  const AbstractModule<bool>& module = *parsed_netlist_ptr->modules().front();

  return LevelSortedCellNames(module);
}

TEST(LevelsortTest, SimpleNetlistLevelsort) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  wire _4_;
  input [2:0] in;
  wire [2:0] in;
  output [2:0] out;
  wire [2:0] out;
  and2 _3_ ( .A(in[1]), .B(_0_), .Y(_1_));
  or2 _2_ ( .A(in[0]), .B(in[1]), .Y(_0_));
  and2 _5_ ( .A(_1_), .B(_0_), .Y(_4_));
  assign { out[2:0] } = { _1_, _0_ };
endmodule
)netlist";

  absl::StatusOr<std::vector<std::vector<std::string>>> level_sorted =
      run_levelsort(netlist);
  EXPECT_THAT(level_sorted, IsOk());
  std::vector<std::vector<std::string>> level_unwrapped = level_sorted.value();
  EXPECT_EQ(level_unwrapped.size(), 3);
  EXPECT_THAT(level_unwrapped[0], UnorderedElementsAre("_2_"));
  EXPECT_THAT(level_unwrapped[1], UnorderedElementsAre("_3_"));
  // The and gate at cell 4 has no wire dependency with another cell, so this
  // tests that unconnected cells still get added to the graph.
  EXPECT_THAT(level_unwrapped[2], UnorderedElementsAre("_5_"));
}

TEST(ToposortTest, SimpleToposort) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [2:0] in;
  wire [2:0] in;
  output [1:0] out;
  wire [2:0] out;
  and2 _3_ ( .A(in[1]), .B(_0_), .Y(_1_));
  or2 _2_ ( .A(in[0]), .B(in[1]), .Y(_0_));
  assign { out[1:0] } = { _1_, _0_ };
endmodule
)netlist";

  EXPECT_THAT(run_toposort(netlist), IsOkAndHolds(ElementsAre("_2_", "_3_")));
}

TEST(ToposortTest, ToposortFailsWithUninitializedWire) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  wire _5_;
  input [2:0] in;
  wire [2:0] in;
  output [1:0] out;
  wire [2:0] out;
  and2 _3_ ( .A(in[1]), .B(_0_), .Y(_1_));
  or2 _2_ ( .A(_5_), .B(in[1]), .Y(_0_));
  assign { out[1:0] } = { _1_, _0_ };
endmodule
)netlist";

  EXPECT_THAT(run_toposort(netlist),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       testing::HasSubstr("usage of uninitialized wire")));
}

TEST(ToposortTest, HandlesWireToWireAssignmentBetweenCells) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  input [2:0] in;
  wire [2:0] in;
  output [1:0] out;
  wire [2:0] out;
  wire _0_;
  wire _1_;
  wire _6_;
  wire _7_;
  or2 _2_ ( .A(in[0]), .B(in[1]), .Y(_0_));
  assign _6_ = _0_;
  assign _7_ = _6_;
  and2 _3_ ( .A(_7_), .B(in[1]), .Y(_1_));
  assign { out[1:0] } = { _1_, _0_ };
endmodule
)netlist";
  EXPECT_THAT(run_toposort(netlist), IsOk());
}

TEST(ToposortTest, HandlesWireToWireAssignmentInNonToposortedStatementOrder) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  input [2:0] in;
  wire [2:0] in;
  output [1:0] out;
  wire [2:0] out;
  wire _0_;
  wire _1_;
  wire _6_;
  wire _7_;
  or2 _2_ ( .A(in[0]), .B(in[1]), .Y(_0_));
  and2 _3_ ( .A(_7_), .B(in[1]), .Y(_1_));
  assign _6_ = _0_;
  assign _7_ = _6_;
  assign { out[1:0] } = { _1_, _0_ };
endmodule
)netlist";
  EXPECT_THAT(run_toposort(netlist), IsOk());
}

TEST(ToposortTest, IncludesCellWithoutWire) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [2:0] in;
  wire [2:0] in;
  output [1:0] out;
  wire [2:0] out;
  and2 _3_ ( .A(in[1]), .B(in[0]), .Y(out[0]));
endmodule
)netlist";

  EXPECT_THAT(run_toposort(netlist), IsOkAndHolds(ElementsAre("_3_")));
}

TEST(ToposortTest, ConstantIsNotBreaking) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [2:0] in;
  wire [2:0] in;
  output [1:0] out;
  wire [2:0] out;
  or2 _4_ ( .A(1'h1), .B(_0_), .Y(_1_));
  and2 _3_ ( .A(1'h0), .B(1'h1), .Y(_0_));
endmodule
)netlist";

  EXPECT_THAT(run_toposort(netlist), IsOkAndHolds(ElementsAre("_3_", "_4_")));
}

TEST(ToposortTest, CyclicNetlistFails) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [2:0] in;
  wire [2:0] in;
  output [1:0] out;
  wire [2:0] out;
  or2 _4_ ( .A(1'h1), .B(_0_), .Y(_1_));
  and2 _3_ ( .A(1'h0), .B(_1_), .Y(_0_));
endmodule
)netlist";

  EXPECT_THAT(run_toposort(netlist),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(LevelSortedCellNamesTest, OutputWireIsReused) {
  // In this test, a gate assigns to an output wire, and that wire is then
  // reused by another gate as an input. This should force the two cells to be
  // in different levels.
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [2:0] in;
  wire [2:0] in;
  output [1:0] out;
  wire [2:0] out;
  and2 _3_ ( .A(in[1]), .B(in[0]), .Y(out[0]));
  and2 _4_ ( .A(out[0]), .B(in[0]), .Y(out[1]));
endmodule
)netlist";

  absl::StatusOr<std::vector<std::vector<std::string>>> level_sorted =
      run_levelsort(netlist);
  EXPECT_THAT(level_sorted, ::xls::status_testing::IsOk());
  std::vector<std::vector<std::string>> level_unwrapped = level_sorted.value();
  EXPECT_EQ(level_unwrapped.size(), 2);
  EXPECT_THAT(level_unwrapped[0], UnorderedElementsAre("_3_"));
  // The and gate at cell 4 has no wire dependency with another cell, so this
  // tests that unconnected cells still get added to the graph.
  EXPECT_THAT(level_unwrapped[1], UnorderedElementsAre("_4_"));
}

TEST(NetRefIdToNumericIdTest, Convert7) {
  EXPECT_THAT(NetRefIdToNumericId("_7_"), IsOkAndHolds(Eq(7)));
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
  EXPECT_THAT(ConstantToValue("<constant_wat>"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ConstantToValueTest, FailConvertImproperlyStructured) {
  EXPECT_THAT(ConstantToValue("constant_7"),
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
