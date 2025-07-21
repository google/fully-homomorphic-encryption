#include "transpiler/jaxite/yosys_transpiler.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_cat.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "xls/common/status/matchers.h"

namespace fhe {
namespace jaxite {
namespace transpiler {
namespace {
using absl_testing::StatusIs;

constexpr absl::string_view kCells = R"lib(
library(testlib) {
  cell(and2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A * B)";}}
  cell(andny2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "((A') * B)";}}
  cell(andyn2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A * (B'))";}}
  cell(buffer) {pin(A) {direction : input;} pin(Y) {direction: output; function : "A";}}
  cell(inv) {pin(A) {direction : input;} pin(Y) {direction : output; function : "A'";}}
  cell(imux2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(S) {direction : input;} pin(Y) {direction: output; function : "(A * S) + (B * (S'))";}}
  cell(nand2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A * B)'";}}
  cell(nor2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A + B)'";}}
  cell(or2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A + B)";}}
  cell(orny2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "((A') + B)";}}
  cell(oryn2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A + (B'))";}}
  cell(xnor2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "((A * (B')) + ((A') * B))'";}}
  cell(xor2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A * (B')) + ((A') * B)";}}
  cell(lut1) { pin(A) { direction : input; } pin(P0) { direction : input; } pin(P1) { direction : input; } pin(Y) { direction: output; function : "(A' P0) + (A P1)"; } }
  cell(lut2) { pin(A) { direction : input; } pin(B) { direction : input; } pin(P0) { direction : input; } pin(P1) { direction : input; } pin(P2) { direction : input; } pin(P3) { direction : input; } pin(Y) { direction: output; function : "(B' A' P0) + (B' A P1) + (B A' P2) + (B A P3)"; } }
  cell(lut3) { pin(A) { direction : input; } pin(B) { direction : input; } pin(C) { direction : input; } pin(P0) { direction : input; } pin(P1) { direction : input; } pin(P2) { direction : input; } pin(P3) { direction : input; } pin(P4) { direction : input; } pin(P5) { direction : input; } pin(P6) { direction : input; } pin(P7) { direction : input; } pin(Y) { direction: output; function : "(C' B' A' P0) | (C' B' A P1) | (C' B A' P2) | (C' B A P3) | (C B' A' P4) | (C B' A P5) | (C B A' P6) | (C B A P7)"; } }
  cell(lut4) { pin(A) { direction : input; } pin(B) { direction : input; } pin(C) { direction : input; } pin(D) { direction : input; } pin(P0) { direction : input; } pin(P1) { direction : input; } pin(P2) { direction : input; } pin(P3) { direction : input; } pin(P4) { direction : input; } pin(P5) { direction : input; } pin(P6) { direction : input; } pin(P7) { direction : input; } pin(P8) { direction : input; } pin(P9) { direction : input; } pin(P10) { direction : input; } pin(P11) { direction : input; } pin(P12) { direction : input; } pin(P13) { direction : input; } pin(P14) { direction : input; } pin(P15) { direction : input; } pin(Y) { direction: output; function : "(D' C' B' A' P0) | (D' C' B' A P1) | (D' C' B A' P2) | (D' C' B A P3) | (D' C B' A' P4) | (D' C B' A P5) | (D' C B A' P6) | (D' C B A P7) | (D C' B' A' P8) | (D C' B' A P9) | (D C' B A' P10) | (D C' B A P11) | (D C B' A' P12) | (D C B' A P13) | (D C B A' P14) | (D C B A P15)"; } }
  cell(lut5) { pin(A) { direction : input; } pin(B) { direction : input; } pin(C) { direction : input; } pin(D) { direction : input; } pin(E) { direction : input; } pin(P0) { direction : input; } pin(P1) { direction : input; } pin(P2) { direction : input; } pin(P3) { direction : input; } pin(P4) { direction : input; } pin(P5) { direction : input; } pin(P6) { direction : input; } pin(P7) { direction : input; } pin(P8) { direction : input; } pin(P9) { direction : input; } pin(P10) { direction : input; } pin(P11) { direction : input; } pin(P12) { direction : input; } pin(P13) { direction : input; } pin(P14) { direction : input; } pin(P15) { direction : input; } pin(P16) { direction : input; } pin(P17) { direction : input; } pin(P18) { direction : input; } pin(P19) { direction : input; } pin(P20) { direction : input; } pin(P21) { direction : input; } pin(P22) { direction : input; } pin(P23) { direction : input; } pin(P24) { direction : input; } pin(P25) { direction : input; } pin(P26) { direction : input; } pin(P27) { direction : input; } pin(P28) { direction : input; } pin(P29) { direction : input; } pin(P30) { direction : input; } pin(P31) { direction : input; } pin(Y) { direction: output; function : "(E' D' C' B' A' P0) | (E' D' C' B' A P1) | (E' D' C' B A' P2) | (E' D' C' B A P3) | (E' D' C B' A' P4) | (E' D' C B' A P5) | (E' D' C B A' P6) | (E' D' C B A P7) | (E' D C' B' A' P8) | (E' D C' B' A P9) | (E' D C' B A' P10) | (E' D C' B A P11) | (E' D C B' A' P12) | (E' D C B' A P13) | (E' D C B A' P14) | (E' D C B A P15) | (E D' C' B' A' P16) | (E D' C' B' A P17) | (E D' C' B A' P18) | (E D' C' B A P19) | (E D' C B' A' P20) | (E D' C B' A P21) | (E D' C B A' P22) | (E D' C B A P23) | (E D C' B' A' P24) | (E D C' B' A P25) | (E D C' B A' P26) | (E D C' B A P27) | (E D C B' A' P28) | (E D C B' A P29) | (E D C B A' P30) | (E D C B A P31)"; } }
  cell(lut6) { pin(A) { direction : input; } pin(B) { direction : input; } pin(C) { direction : input; } pin(D) { direction : input; } pin(E) { direction : input; } pin(F) { direction : input; } pin(P0) { direction : input; } pin(P1) { direction : input; } pin(P2) { direction : input; } pin(P3) { direction : input; } pin(P4) { direction : input; } pin(P5) { direction : input; } pin(P6) { direction : input; } pin(P7) { direction : input; } pin(P8) { direction : input; } pin(P9) { direction : input; } pin(P10) { direction : input; } pin(P11) { direction : input; } pin(P12) { direction : input; } pin(P13) { direction : input; } pin(P14) { direction : input; } pin(P15) { direction : input; } pin(P16) { direction : input; } pin(P17) { direction : input; } pin(P18) { direction : input; } pin(P19) { direction : input; } pin(P20) { direction : input; } pin(P21) { direction : input; } pin(P22) { direction : input; } pin(P23) { direction : input; } pin(P24) { direction : input; } pin(P25) { direction : input; } pin(P26) { direction : input; } pin(P27) { direction : input; } pin(P28) { direction : input; } pin(P29) { direction : input; } pin(P30) { direction : input; } pin(P31) { direction : input; } pin(P32) { direction : input; } pin(P33) { direction : input; } pin(P34) { direction : input; } pin(P35) { direction : input; } pin(P36) { direction : input; } pin(P37) { direction : input; } pin(P38) { direction : input; } pin(P39) { direction : input; } pin(P40) { direction : input; } pin(P41) { direction : input; } pin(P42) { direction : input; } pin(P43) { direction : input; } pin(P44) { direction : input; } pin(P45) { direction : input; } pin(P46) { direction : input; } pin(P47) { direction : input; } pin(P48) { direction : input; } pin(P49) { direction : input; } pin(P50) { direction : input; } pin(P51) { direction : input; } pin(P52) { direction : input; } pin(P53) { direction : input; } pin(P54) { direction : input; } pin(P55) { direction : input; } pin(P56) { direction : input; } pin(P57) { direction : input; } pin(P58) { direction : input; } pin(P59) { direction : input; } pin(P60) { direction : input; } pin(P61) { direction : input; } pin(P62) { direction : input; } pin(P63) { direction : input; } pin(Y) { direction: output; function : "(F' E' D' C' B' A' P0) | (F' E' D' C' B' A P1) | (F' E' D' C' B A' P2) | (F' E' D' C' B A P3) | (F' E' D' C B' A' P4) | (F' E' D' C B' A P5) | (F' E' D' C B A' P6) | (F' E' D' C B A P7) | (F' E' D C' B' A' P8) | (F' E' D C' B' A P9) | (F' E' D C' B A' P10) | (F' E' D C' B A P11) | (F' E' D C B' A' P12) | (F' E' D C B' A P13) | (F' E' D C B A' P14) | (F' E' D C B A P15) | (F' E D' C' B' A' P16) | (F' E D' C' B' A P17) | (F' E D' C' B A' P18) | (F' E D' C' B A P19) | (F' E D' C B' A' P20) | (F' E D' C B' A P21) | (F' E D' C B A' P22) | (F' E D' C B A P23) | (F' E D C' B' A' P24) | (F' E D C' B' A P25) | (F' E D C' B A' P26) | (F' E D C' B A P27) | (F' E D C B' A' P28) | (F' E D C B' A P29) | (F' E D C B A' P30) | (F' E D C B A P31) | (F E' D' C' B' A' P32) | (F E' D' C' B' A P33) | (F E' D' C' B A' P34) | (F E' D' C' B A P35) | (F E' D' C B' A' P36) | (F E' D' C B' A P37) | (F E' D' C B A' P38) | (F E' D' C B A P39) | (F E' D C' B' A' P40) | (F E' D C' B' A P41) | (F E' D C' B A' P42) | (F E' D C' B A P43) | (F E' D C B' A' P44) | (F E' D C B' A P45) | (F E' D C B A' P46) | (F E' D C B A P47) | (F E D' C' B' A' P48) | (F E D' C' B' A P49) | (F E D' C' B A' P50) | (F E D' C' B A P51) | (F E D' C B' A' P52) | (F E D' C B' A P53) | (F E D' C B A' P54) | (F E D' C B A P55) | (F E D C' B' A' P56) | (F E D C' B' A P57) | (F E D C' B A' P58) | (F E D C' B A P59) | (F E D C B' A' P60) | (F E D C B' A P61) | (F E D C B A' P62) | (F E D C B A P63)"; } }
}
)lib";

constexpr absl::string_view kExpectedPrelude =
    R"(from typing import Dict, List

from jaxite.jaxite_bool import jaxite_bool
from jaxite.jaxite_lib import types

)";

TEST(YosysTranspilerTest, TranslateSimpleFunction) {
  constexpr absl::string_view cells = R"lib(
library(testlib) {
  cell(inv) {
    pin(A) { direction : input; }
    pin(Y) { direction : output; function : "A'"; }
  }
  cell(and2) {
    pin(A) { direction : input; }
    pin(B) { direction : input; }
    pin(Y) { direction: output; function : "(A * B)"; }
  }
}
)lib";
  // In the netlist spec, the strings `_0_`, `_1_`, etc. correspond to the
  // `name` field of an AbstractCell, while the gate operations `inv` and `and2`
  // correspond to cell_library_entry.name.
  constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [2:0] in;
  wire [2:0] in;
  output [1:0] out;
  wire [2:0] out;
  inv _2_ ( .A(in[0]), .Y(_0_));
  and2 _3_ ( .A(in[1]), .B(_0_), .Y(_1_));
  assign { out[1:0] } = { _1_, _0_ };
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(cells, netlist));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def test_module(in: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> List[types.LweCiphertext]:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  out = [None] * 2
  temp_nodes[0] = jaxite_bool.not_(in[0], params)
  temp_nodes[1] = jaxite_bool.and_(in[1], temp_nodes[0], sks, params)
  out[0] = temp_nodes[0]
  out[1] = temp_nodes[1]
  return out
)python"));
}

TEST(YosysTranspilerTest, TranslateAllGates) {
  constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  wire _2_;
  wire _3_;
  wire _4_;
  wire _5_;
  wire _6_;
  wire _7_;
  wire _8_;
  wire _9_;
  wire _10_;
  wire _11_;
  wire _12_;
  wire _13_;
  wire _14_;
  wire _15_;
  wire _16_;
  wire _17_;
  wire _18_;
  input [2:0] in;
  wire [2:0] in;
  output [1:0] out;
  wire [1:0] out;
  inv _2_ ( .A(in[0]), .Y(_0_));
  and2 _3_ ( .A(in[1]), .B(_0_), .Y(_1_));
  andny2 _4_ ( .A(in[1]), .B(_1_), .Y(_2_));
  andyn2 _5_ ( .A(in[1]), .B(_2_), .Y(_3_));
  nand2 _6_ ( .A(in[1]), .B(_3_), .Y(_4_));
  nor2 _7_ ( .A(in[1]), .B(_4_), .Y(_5_));
  or2 _8_ ( .A(in[1]), .B(_5_), .Y(_6_));
  orny2 _9_ ( .A(in[1]), .B(_6_), .Y(_7_));
  oryn2 _10_ ( .A(in[1]), .B(_7_), .Y(_8_));
  xnor2 _11_ ( .A(in[1]), .B(_8_), .Y(_9_));
  xor2 _12_ ( .A(in[1]), .B(_9_), .Y(_10_));
  imux2 _13_ ( .A(in[1]), .B(_10_), .S(_1_), .Y(_11_));
  lut2 _14_ ( .A(in[1]), .B(_11_), .P0(1), .P1(0), .P2(1), .P3(1), .Y(_12_));
  lut3 _15_ ( .A(in[1]), .B(_12_), .C(_0_), .P0(1), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_13_));
  lut4 _16_ ( .A(in[1]), .B(_12_), .C(_0_), .D(_0_),
    .P0(0), .P1(1), .P2(1), .P3(1), .P4(1), .P5(1), .P6(1), .P7(1), .P8(1), .P9(1), .P10(1), .P11(1), .P12(1), .P13(1), .P14(1), .P15(1),
    .Y(_14_));
  lut5 _17_ ( .A(in[1]), .B(_12_), .C(_0_), .D(_0_), .E(_0_),
    .P0(0), .P1(1), .P2(1), .P3(1), .P4(1), .P5(1), .P6(1), .P7(1), .P8(1), .P9(1), .P10(1), .P11(1), .P12(1), .P13(1), .P14(1), .P15(1), .P16(1), .P17(1), .P18(1), .P19(1), .P20(1), .P21(1), .P22(1), .P23(1), .P24(1), .P25(1), .P26(1), .P27(1), .P28(1), .P29(1), .P30(1), .P31(1),
    .Y(_15_));
  lut6 _18_ ( .A(in[1]), .B(_12_), .C(_0_), .D(_0_), .E(_0_), .F(_0_),
    .P0(0), .P1(1), .P2(1), .P3(1), .P4(1), .P5(1), .P6(1), .P7(1), .P8(1), .P9(1), .P10(1), .P11(1), .P12(1), .P13(1), .P14(1), .P15(1), .P16(1), .P17(1), .P18(1), .P19(1), .P20(1), .P21(1), .P22(1), .P23(1), .P24(1), .P25(1), .P26(1), .P27(1), .P28(1), .P29(1), .P30(1), .P31(1), .P32(1), .P33(1), .P34(1), .P35(1), .P36(1), .P37(1), .P38(1), .P39(1), .P40(1), .P41(1), .P42(1), .P43(1), .P44(1), .P45(1), .P46(1), .P47(1), .P48(1), .P49(1), .P50(1), .P51(1), .P52(1), .P53(1), .P54(1), .P55(1), .P56(1), .P57(1), .P58(1), .P59(1), .P60(1), .P61(1), .P62(1), .P63(1),
    .Y(_16_));
  assign { out[1:0] } = { _13_, _12_ };
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(kCells, netlist));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def test_module(in: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> List[types.LweCiphertext]:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  out = [None] * 2
  temp_nodes[0] = jaxite_bool.not_(in[0], params)
  temp_nodes[1] = jaxite_bool.and_(in[1], temp_nodes[0], sks, params)
  temp_nodes[2] = jaxite_bool.andny_(in[1], temp_nodes[1], sks, params)
  temp_nodes[3] = jaxite_bool.andyn_(in[1], temp_nodes[2], sks, params)
  temp_nodes[4] = jaxite_bool.nand_(in[1], temp_nodes[3], sks, params)
  temp_nodes[5] = jaxite_bool.nor_(in[1], temp_nodes[4], sks, params)
  temp_nodes[6] = jaxite_bool.or_(in[1], temp_nodes[5], sks, params)
  temp_nodes[7] = jaxite_bool.orny_(in[1], temp_nodes[6], sks, params)
  temp_nodes[8] = jaxite_bool.oryn_(in[1], temp_nodes[7], sks, params)
  temp_nodes[9] = jaxite_bool.xnor_(in[1], temp_nodes[8], sks, params)
  temp_nodes[10] = jaxite_bool.xor_(in[1], temp_nodes[9], sks, params)
  temp_nodes[11] = jaxite_bool.cmux_(in[1], temp_nodes[10], temp_nodes[1], sks, params)
  temp_nodes[12] = jaxite_bool.lut2(in[1], temp_nodes[11], 13, sks, params)
  temp_nodes[16] = jaxite_bool.lut6(in[1], temp_nodes[12], temp_nodes[0], temp_nodes[0], temp_nodes[0], temp_nodes[0], 18446744073709551614, sks, params)
  temp_nodes[15] = jaxite_bool.lut5(in[1], temp_nodes[12], temp_nodes[0], temp_nodes[0], temp_nodes[0], 4294967294, sks, params)
  temp_nodes[14] = jaxite_bool.lut4(in[1], temp_nodes[12], temp_nodes[0], temp_nodes[0], 65534, sks, params)
  temp_nodes[13] = jaxite_bool.lut3(in[1], temp_nodes[12], temp_nodes[0], 221, sks, params)
  out[0] = temp_nodes[12]
  out[1] = temp_nodes[13]
  return out
)python"));
}

TEST(YosysTranspilerTest, MultipleModules_InvalidArgument) {
  constexpr absl::string_view kNetlistText = R"netlist(
module TestModule(in, out);
endmodule

module TestModule2(in, out);
endmodule
)netlist";

  EXPECT_THAT(YosysTranspiler::Translate(kCells, kNetlistText),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(YosysTranspilerTest, MultipleCellOuptuts_InvalidArgument) {
  constexpr absl::string_view kCellLibraryText = R"lib(
library(testlib) {
  cell(mycell) {
    pin(A) { direction : input; }
    pin(B) { direction : input; }
    pin(Y) { direction : output; function : "(A * B)"; }
    pin(Z) { direction : output; function : "(A' * B)"; }
  }
}
)lib";

  constexpr absl::string_view kNetlistText = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  wire _2_;
  input [2:0] in;
  wire [2:0] in;
  output [2:0] out;
  wire [2:0] out;
  mycell _3_ ( .A(in[1]), .B(in[0]), .Y(_1_), .Z(_2_));

endmodule
)netlist";

  EXPECT_THAT(YosysTranspiler::Translate(kCellLibraryText, kNetlistText),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(YosysTranspiler::Translate(kCellLibraryText, kNetlistText, 5),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(YosysTranspilerTest, MultipleModuleOuptuts_InvalidArgument) {
  constexpr absl::string_view kCellLibraryText = R"lib(
library(testlib) {
  cell(mycell) {
    pin(A) { direction : input; }
    pin(B) { direction : input; }
    pin(Y) { direction : output; function : "(A * B)"; }
  }
}
)lib";
  constexpr absl::string_view kNetlistText = R"netlist(
module TestModule(in, out1, out2);
  wire _0_;
  wire _1_;
  wire _2_;
  input [2:0] in;
  wire [2:0] in;
  output [2:0] out1;
  output [2:0] out2;
  wire [2:0] out;
  mycell _3_ ( .A(in[1]), .B(_0_), .Y(_1_));

endmodule
)netlist";

  EXPECT_THAT(YosysTranspiler::Translate(kCellLibraryText, kNetlistText),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(YosysTranspilerTest, UnknownGateMapping_InvalidArgument) {
  // Even though mycell is in the cell library, there's a hard coded mapping in
  // the codegen step to tfhe_jax, and `mycell` is not in that mapping.
  constexpr absl::string_view kCellLibraryText = R"lib(
library(testlib) {
  cell(mycell) {
    pin(A) { direction : input; }
    pin(B) { direction : input; }
    pin(Y) { direction : output; function : "(A * B)"; }
  }
}
)lib";
  constexpr absl::string_view kNetlistText = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [2:0] in;
  wire [2:0] in;
  output [2:0] out;
  wire [2:0] out;
  mycell _3_ ( .A(in[1]), .B(in[0]), .Y(_1_));
endmodule
)netlist";

  EXPECT_THAT(YosysTranspiler::Translate(kCellLibraryText, kNetlistText),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       testing::HasSubstr("unknown codegen mapping")));
}

TEST(YosysTranspilerTest, NonNumericWireName_ParsingFails) {
  constexpr absl::string_view kNetlistText =
      R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _foo_;
  wire _2_;
  input [2:0] in;
  wire [2:0] in;
  output [2:0] out;
  wire [2:0] out;
  and2 _3_ ( .A(in[1]), .B(_0_), .Y(_foo_));
endmodule
)netlist";

  EXPECT_THAT(YosysTranspiler::Translate(kCells, kNetlistText),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(YosysTranspilerTest, YosysOutputComment_ParsesSuccessfully) {
  constexpr absl::string_view kNetlistText =
      R"netlist(/* Generated by Yosys */
module TestModule(in, out);
  wire _0_;
  wire _1_;
  wire _2_;
  input [2:0] in;
  wire [2:0] in;
  output [2:0] out;
  wire [2:0] out;
  and2 _3_ ( .A(in[1]), .B(in[0]), .Y(_1_));
endmodule
)netlist";

  EXPECT_THAT(YosysTranspiler::Translate(kCells, kNetlistText),
              StatusIs(absl::StatusCode::kOk));
}

TEST(YosysTranspilerTest, LeadingZerosOnWireNames_TruncatesToInt) {
  // E.g., the _01_ value in the output of the and2 cell must have its leading
  // zeros truncated, because in Python the literal 0123 is illegal (it warns to
  // use the proper octal base notation, which we don't want in this case
  // anyway).
  constexpr absl::string_view kNetlistText =
      R"netlist(
module TestModule(in, out);
  wire _00_;
  wire _01_;
  wire _02_;
  input [2:0] in;
  wire [2:0] in;
  output [2:0] out;
  wire [2:0] out;
  and2 _03_ ( .A(in[1]), .B(in[0]), .Y(_00_));
  and2 _04_ ( .A(in[1]), .B(_00_), .Y(_01_));
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(kCells, kNetlistText));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def test_module(in: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> List[types.LweCiphertext]:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  out = [None] * 3
  temp_nodes[0] = jaxite_bool.and_(in[1], in[0], sks, params)
  temp_nodes[1] = jaxite_bool.and_(in[1], temp_nodes[0], sks, params)
  return out
)python"));
}

TEST(YosysTranspilerTest, ConstantsInInputPins_ParsesSuccessfully) {
  constexpr absl::string_view kNetlistText =
      R"netlist(
module TestModule(in, out);
  wire _00_;
  wire _01_;
  wire _02_;
  input [2:0] in;
  wire [2:0] in;
  output [2:0] out;
  wire [2:0] out;
  and2 _03_ ( .A(in[1]), .B(1'h0), .Y(_00_));
  and2 _04_ ( .A(in[1]), .B(1'h1), .Y(_01_));
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(kCells, kNetlistText));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def test_module(in: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> List[types.LweCiphertext]:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  out = [None] * 3
  temp_nodes[1] = jaxite_bool.and_(in[1], jaxite_bool.constant(True, params), sks, params)
  temp_nodes[0] = jaxite_bool.and_(in[1], jaxite_bool.constant(False, params), sks, params)
  return out
)python"));
}

TEST(YosysTranspilerTest, CellOutputPinIsOutput_TranslatesSuccessfully) {
  // When a pin uses `Y(out[1])` instead of `Y(_05_)` followed by an assignment,
  // we need to ensure we output the LHS of the corresponding Python assignment
  // properly (i.e., not use temp_nodes). Whether the Yosys output includes
  // assignment statements for the final output or not seems to depend on the
  // specific script used.
  constexpr absl::string_view kNetlistText =
      R"netlist(
module TestModule(in, out);
  wire _00_;
  wire _01_;
  wire _02_;
  input [2:0] in;
  wire [2:0] in;
  output [2:0] out;
  wire [2:0] out;
  and2 _03_ ( .A(in[1]), .B(in[0]), .Y(out[1]));
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(kCells, kNetlistText));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def test_module(in: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> List[types.LweCiphertext]:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  out = [None] * 3
  out[1] = jaxite_bool.and_(in[1], in[0], sks, params)
  return out
)python"));
}

TEST(YosysTranspilerTest, Buffer_ConvertsToAssignment) {
  constexpr absl::string_view kNetlistText =
      R"netlist(
module TestModule(in, out);
  input [2:0] in;
  wire [2:0] in;
  output [2:0] out;
  wire [2:0] out;
  buffer _03_ ( .A(in[1]), .Y(out[1]));
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(kCells, kNetlistText));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def test_module(in: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> List[types.LweCiphertext]:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  out = [None] * 3
  out[1] = in[1]
  return out
)python"));
}

TEST(YosysTranspilerTest, SingleBitOutput_NoListInstantiation) {
  constexpr absl::string_view kNetlistText =
      R"netlist(
module TestModule(in, out);
  input [2:0] in;
  wire [2:0] in;
  output out;
  wire out;
  buffer _03_ ( .A(in[1]), .Y(out));
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(kCells, kNetlistText));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def test_module(in: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> types.LweCiphertext:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  out = in[1]
  return out
)python"));
}

TEST(YosysTranspilerTest, SingleBitInput_NoListInstantiation) {
  constexpr absl::string_view kNetlistText =
      R"netlist(
module TestModule(in, out);
  input in;
  wire in;
  output out;
  wire out;
  buffer _03_ ( .A(in), .Y(out));
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(kCells, kNetlistText));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def test_module(in: types.LweCiphertext, sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> types.LweCiphertext:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  out = in
  return out
)python"));
}

TEST(YosysTranspilerTest, TopoSortsNetlist) {
  // This netlist was the (non-topo-sorted) intermediate output of the
  // transpiler for transpiler/examples/add_one. Note that cell _06_ is the cell
  // connected to wire _00_, but the cell just before it (_05_) has _00_ as it's
  // A  input.
  constexpr absl::string_view kNetlistText =
      R"netlist(
module add_one(x, out);
  wire _00_;
  wire _01_;
  wire _02_;
  output [7:0] out;
  wire [7:0] out;
  input [7:0] x;
  wire [7:0] x;
  lut2 _03_ ( .A(x[0]), .B(x[1]), .P0(1'h0), .P1(1'h1), .P2(1'h1), .P3(1'h0), .Y(out[1]));
  lut3 _04_ ( .A(x[0]), .B(x[1]), .C(x[2]), .P0(1'h0), .P1(1'h0), .P2(1'h0), .P3(1'h1), .P4(1'h1), .P5(1'h1), .P6(1'h1), .P7(1'h0), .Y(out[2]));
  lut2 _05_ ( .A(_00_), .B(x[3]), .P0(1'h0), .P1(1'h1), .P2(1'h1), .P3(1'h0), .Y(out[3]));
  lut3 _06_ ( .A(x[0]), .B(x[1]), .C(x[2]), .P0(1'h0), .P1(1'h0), .P2(1'h0), .P3(1'h0), .P4(1'h0), .P5(1'h0), .P6(1'h0), .P7(1'h1), .Y(_00_));
  lut3 _07_ ( .A(_00_), .B(x[3]), .C(x[4]), .P0(1'h0), .P1(1'h0), .P2(1'h0), .P3(1'h1), .P4(1'h1), .P5(1'h1), .P6(1'h1), .P7(1'h0), .Y(out[4]));
  lut2 _08_ ( .A(_01_), .B(x[5]), .P0(1'h0), .P1(1'h1), .P2(1'h1), .P3(1'h0), .Y(out[5]));
  lut3 _09_ ( .A(_00_), .B(x[3]), .C(x[4]), .P0(1'h0), .P1(1'h0), .P2(1'h0), .P3(1'h0), .P4(1'h0), .P5(1'h0), .P6(1'h0), .P7(1'h1), .Y(_01_));
  lut3 _10_ ( .A(_01_), .B(x[5]), .C(x[6]), .P0(1'h0), .P1(1'h0), .P2(1'h0), .P3(1'h1), .P4(1'h1), .P5(1'h1), .P6(1'h1), .P7(1'h0), .Y(out[6]));
  lut2 _11_ ( .A(_02_), .B(x[7]), .P0(1'h0), .P1(1'h1), .P2(1'h1), .P3(1'h0), .Y(out[7]));
  lut3 _12_ ( .A(_01_), .B(x[5]), .C(x[6]), .P0(1'h0), .P1(1'h0), .P2(1'h0), .P3(1'h0), .P4(1'h0), .P5(1'h0), .P6(1'h0), .P7(1'h1), .Y(_02_));
  lut1 _13_ ( .A(x[0]), .P0(1'h1), .P1(1'h0), .Y(out[0]));
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(kCells, kNetlistText));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def add_one(x: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> List[types.LweCiphertext]:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  out = [None] * 8
  out[0] = jaxite_bool.lut1(x[0], 1, sks, params)
  temp_nodes[0] = jaxite_bool.lut3(x[0], x[1], x[2], 128, sks, params)
  temp_nodes[1] = jaxite_bool.lut3(temp_nodes[0], x[3], x[4], 128, sks, params)
  temp_nodes[2] = jaxite_bool.lut3(temp_nodes[1], x[5], x[6], 128, sks, params)
  out[7] = jaxite_bool.lut2(temp_nodes[2], x[7], 6, sks, params)
  out[6] = jaxite_bool.lut3(temp_nodes[1], x[5], x[6], 120, sks, params)
  out[5] = jaxite_bool.lut2(temp_nodes[1], x[5], 6, sks, params)
  out[4] = jaxite_bool.lut3(temp_nodes[0], x[3], x[4], 120, sks, params)
  out[3] = jaxite_bool.lut2(temp_nodes[0], x[3], 6, sks, params)
  out[2] = jaxite_bool.lut3(x[0], x[1], x[2], 120, sks, params)
  out[1] = jaxite_bool.lut2(x[0], x[1], 6, sks, params)
  return out
)python"));
}

TEST(YosysTranspilerTest, ParallelizeCircuitSingleLevel) {
  constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [2:0] in;
  wire [2:0] in;
  output out;
  wire out;
  lut3 _3_ ( .A(in[1]), .B(in[0]), .C(in[0]), .P0(1), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_0_));
  lut3 _4_ ( .A(in[1]), .B(in[0]), .C(in[1]), .P0(0), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_1_));
  assign { out } = { _0_ };
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(kCells, netlist, 8));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def test_module(in: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> types.LweCiphertext:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  inputs = [
    (in[1], in[0], in[0], 221),
    (in[1], in[0], in[1], 220),
  ]
  outputs = jaxite_bool.pmap_lut3(inputs, sks, params)
  temp_nodes[0] = outputs[0]
  temp_nodes[1] = outputs[1]
  out = temp_nodes[0]
  return out
)python"));
}

TEST(YosysTranspilerTest, ParallelizeCircuitTwoLevels) {
  constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  wire _2_;
  input [2:0] in;
  wire [2:0] in;
  output out;
  wire out;
  lut3 _3_ ( .A(in[1]), .B(in[0]), .C(in[0]), .P0(1), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_0_));
  lut3 _4_ ( .A(in[1]), .B(in[0]), .C(in[1]), .P0(0), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_1_));
  lut3 _5_ ( .A(_0_), .B(_1_), .C(in[1]), .P0(0), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_2_));
  assign { out } = { _2_ };
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(kCells, netlist, 8));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def test_module(in: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> types.LweCiphertext:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  inputs = [
    (in[1], in[0], in[0], 221),
    (in[1], in[0], in[1], 220),
  ]
  outputs = jaxite_bool.pmap_lut3(inputs, sks, params)
  temp_nodes[0] = outputs[0]
  temp_nodes[1] = outputs[1]
  inputs = [
    (temp_nodes[0], temp_nodes[1], in[1], 220),
  ]
  outputs = jaxite_bool.pmap_lut3(inputs, sks, params)
  temp_nodes[2] = outputs[0]
  out = temp_nodes[2]
  return out
)python"));
}

TEST(YosysTranspilerTest, ParallelizeCircuitOneLevelTwoBatches) {
  constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  wire _2_;
  wire _3_;
  wire _4_;
  wire _5_;
  input [2:0] in;
  wire [2:0] in;
  output out;
  wire out;
  lut3 _6_ ( .A(in[1]), .B(in[0]), .C(in[0]), .P0(1), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_0_));
  lut3 _7_ ( .A(in[1]), .B(in[0]), .C(in[0]), .P0(1), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_1_));
  lut3 _8_ ( .A(in[1]), .B(in[0]), .C(in[0]), .P0(1), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_2_));
  lut3 _9_ ( .A(in[1]), .B(in[0]), .C(in[0]), .P0(1), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_3_));
  lut3 _10_ ( .A(in[1]), .B(in[0]), .C(in[0]), .P0(1), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_4_));
  lut3 _20_ ( .A(in[1]), .B(in[0]), .C(in[0]), .P0(1), .P1(0), .P2(1), .P3(1), .P4(1), .P5(0), .P6(1), .P7(1), .Y(_5_));
  assign { out } = { _5_ };
endmodule
)netlist";

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual,
                           YosysTranspiler::Translate(kCells, netlist, 3));
  EXPECT_EQ(
      actual,
      absl::StrCat(
          kExpectedPrelude,
          R"python(def test_module(in: List[types.LweCiphertext], sks: jaxite_bool.ServerKeySet, params: jaxite_bool.Parameters) -> types.LweCiphertext:
  temp_nodes: Dict[int, types.LweCiphertext] = {}
  inputs = [
    (in[1], in[0], in[0], 221),
    (in[1], in[0], in[0], 221),
    (in[1], in[0], in[0], 221),
  ]
  outputs = jaxite_bool.pmap_lut3(inputs, sks, params)
  temp_nodes[4] = outputs[0]
  temp_nodes[5] = outputs[1]
  temp_nodes[0] = outputs[2]
  inputs = [
    (in[1], in[0], in[0], 221),
    (in[1], in[0], in[0], 221),
    (in[1], in[0], in[0], 221),
  ]
  outputs = jaxite_bool.pmap_lut3(inputs, sks, params)
  temp_nodes[1] = outputs[0]
  temp_nodes[2] = outputs[1]
  temp_nodes[3] = outputs[2]
  out = temp_nodes[5]
  return out
)python"));
}

}  // namespace
}  // namespace transpiler
}  // namespace jaxite
}  // namespace fhe
