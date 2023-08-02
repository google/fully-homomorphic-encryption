#include "transpiler/rust/yosys_transpiler.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/netlist_utils.h"
#include "transpiler/rust/tfhe_rs_templates.h"
#include "xls/common/status/matchers.h"
#include "xls/netlist/cell_library.h"
#include "xls/netlist/netlist.h"

namespace fhe {
namespace rust {
namespace transpiler {
namespace {

using ::std::vector;

constexpr absl::string_view kCells = R"lib(
library(testlib) {
  cell(and2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A * B)";}}
  cell(inv) {pin(A) {direction : input;} pin(Y) {direction : output; function : "A'";}}
  cell(imux2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(S) {direction : input;} pin(Y) {direction: output; function : "(A * S) + (B * (S'))";}}
  cell(nand2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A * B)'";}}
  cell(nor2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A + B)'";}}
  cell(or2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A + B)";}}
  cell(xnor2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "((A * (B')) + ((A') * B))'";}}
  cell(xor2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A * (B')) + ((A') * B)";}}
  cell(lut3) { pin(A) { direction : input; } pin(B) { direction : input; } pin(C) { direction : input; } pin(P0) { direction : input; } pin(P1) { direction : input; } pin(P2) { direction : input; } pin(P3) { direction : input; } pin(P4) { direction : input; } pin(P5) { direction : input; } pin(P6) { direction : input; } pin(P7) { direction : input; } pin(Y) { direction: output; function : "(C' B' A' P0) | (C' B' A P1) | (C' B A' P2) | (C' B A P3) | (C B' A' P4) | (C B' A P5) | (C B A' P6) | (C B A P7)"; } }
}
)lib";

// A helper for constructing FunctionMetadata protos
struct Parameter {
  std::string name;
  bool in_out = false;
  int bit_width = 8;
};

// Create a test metadata proto with the relevant fields set. All we really care
// about here is in/out params. E.g., to create a function with one 8-bit in/out
// param:
//
// xlscc_metadata::MetadataOutput metadata =
//     CreateFunctionMetadata({{"param", true, 8}}, false);
xlscc_metadata::MetadataOutput CreateFunctionMetadata(
    const vector<Parameter>& params, bool has_return_value,
    int return_bit_width) {
  xlscc_metadata::MetadataOutput output;

  for (const Parameter& param : params) {
    xlscc_metadata::FunctionParameter* xls_param =
        output.mutable_top_func_proto()->add_params();
    xls_param->set_name(param.name);
    xls_param->mutable_type()->mutable_as_int()->set_width(param.bit_width);
    if (param.in_out) {
      xls_param->set_is_reference(true);
      xls_param->set_is_const(false);
    } else {
      xls_param->set_is_const(true);
    }
  }

  if (has_return_value) {
    output.mutable_top_func_proto()
        ->mutable_return_type()
        ->mutable_as_int()
        ->set_width(return_bit_width);
  } else {
    output.mutable_top_func_proto()->mutable_return_type()->mutable_as_void();
  }

  return output;
}

std::string BuildExpectedGateOps(int level_id, vector<std::string> tasks) {
  constexpr absl::string_view kTaskTemplate = "    %s";
  vector<std::string> tasks_formatted;
  for (auto task : tasks)
    tasks_formatted.push_back(absl::StrFormat(kTaskTemplate, task));

  return absl::StrFormat(kLevelTemplate, level_id, tasks.size(),
                         absl::StrJoin(tasks_formatted, "\n"));
}

TEST(YosysTfheRsTranspilerTest, TranslateSimpleFunction) {
  constexpr absl::string_view netlist_text = R"netlist(
module test_module(data, out);
  wire _0_;
  input [2:0] data;
  wire [2:0] data;
  output [1:0] out;
  wire [1:0] out;
  lut3 _2_ ( .A(data[0]), .B(data[1]), .C(data[2]), .P0(1'h0), .P1(1'h0), .P2(1'h0), .P3(1'h1), .P4(1'h1), .P5(1'h1), .P6(1'h1), .P7(1'h0), .Y(_0_));
  assign { out[1:0] } = { data[0], _0_ };
endmodule
)netlist";

  absl::string_view signature =
      "test_module(data: &Vec<Ciphertext>, server_key: &ServerKey) -> "
      "Vec<Ciphertext>";
  vector<int> luts = {120};
  const int num_outputs = 2;
  const int num_gates = 1;
  const absl::string_view output_stem = "out";
  vector<std::string> ordered_params = {"data"};
  vector<std::string> outputs = {"temp_nodes[0]", "data[0]"};

  vector<std::string> tasks = {
      "((0, false, LUT3(120)), &[Arg(0, 0), Arg(0, 1), Arg(0, 2)]),"};
  vector<std::string> output_assignments = {
      "    out[0] = temp_nodes[&0].clone();",
      "    out[1] = data[0].clone();",
  };
  std::string expected = absl::StrReplaceAll(
      kCodegenTemplate,
      {
          {"$gate_levels", BuildExpectedGateOps(0, tasks)},
          {"$function_signature", signature},
          {"$ordered_params", absl::StrJoin(ordered_params, ", ")},
          {"$total_num_gates", absl::StrFormat("%d", num_gates)},
          {"$output_stem", output_stem},
          {"$num_outputs", absl::StrFormat("%d", num_outputs)},
          {"$num_luts", absl::StrFormat("%d", luts.size())},
          {"$comma_separated_luts", absl::StrJoin(luts, ", ")},
          {"$run_level_ops", absl::StrFormat(kRunLevelTemplate, 0)},
          {"$output_assignment_block", absl::StrJoin(output_assignments, "\n")},
          {"$return_statement", output_stem},
      });

  auto metadata = CreateFunctionMetadata(
      {{"data", false, /*return_bit_width=*/3}}, true, num_outputs);
  XLS_ASSERT_OK_AND_ASSIGN(
      xls::netlist::AbstractCellLibrary<bool> cell_library,
      fully_homomorphic_encryption::transpiler::ParseCellLibrary(kCells));
  XLS_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<::xls::netlist::rtl::AbstractNetlist<bool>> netlist,
      fully_homomorphic_encryption::transpiler::ParseNetlist(cell_library,
                                                             netlist_text));
  YosysTfheRsTranspiler transpiler(metadata, std::move(netlist));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual, transpiler.Translate());
  EXPECT_EQ(expected, actual);
}

TEST(YosysTfheRsTranspilerTest, CellOutputsAssignToModuleOutput) {
  constexpr absl::string_view netlist_text = R"netlist(
module test_module(data, out);
  wire _0_;
  input [2:0] data;
  wire [2:0] data;
  output [2:0] out;
  wire [2:0] out;
  lut3 _2_ ( .A(data[0]), .B(data[1]), .C(data[2]), .P0(1'h0), .P1(1'h0), .P2(1'h0), .P3(1'h1), .P4(1'h1), .P5(1'h1), .P6(1'h1), .P7(1'h0), .Y(out[2]));
  lut3 _3_ ( .A(data[0]), .B(out[2]), .C(data[2]), .P0(1'h0), .P1(1'h0), .P2(1'h0), .P3(1'h1), .P4(1'h1), .P5(1'h1), .P6(1'h1), .P7(1'h0), .Y(_0_));
  assign { out[1:0] } = { data[0], _0_ };
endmodule
)netlist";

  absl::string_view signature =
      "test_module(data: &Vec<Ciphertext>, server_key: &ServerKey) -> "
      "Vec<Ciphertext>";
  vector<int> luts = {120};
  const int num_outputs = 3;
  const int num_gates = 2;
  const absl::string_view output_stem = "out";
  vector<std::string> ordered_params = {"data"};

  vector<std::string> tasks0 = {
      "((2, true, LUT3(120)), &[Arg(0, 0), Arg(0, 1), Arg(0, 2)]),"};
  vector<std::string> tasks1 = {
      "((0, false, LUT3(120)), &[Arg(0, 0), Output(2), Arg(0, 2)]),"};

  vector<std::string> gate_levels = {
      BuildExpectedGateOps(0, tasks0),
      BuildExpectedGateOps(1, tasks1),
  };
  vector<std::string> run_level_ops = {
      absl::StrFormat(kRunLevelTemplate, 0),
      absl::StrFormat(kRunLevelTemplate, 1),
  };
  vector<std::string> output_assignments = {
      "    out[0] = temp_nodes[&0].clone();",
      "    out[1] = data[0].clone();",
  };
  std::string expected = absl::StrReplaceAll(
      kCodegenTemplate,
      {
          {"$gate_levels", absl::StrJoin(gate_levels, "\n")},
          {"$function_signature", signature},
          {"$ordered_params", absl::StrJoin(ordered_params, ", ")},
          {"$total_num_gates", absl::StrFormat("%d", num_gates)},
          {"$output_stem", output_stem},
          {"$num_outputs", absl::StrFormat("%d", num_outputs)},
          {"$num_luts", absl::StrFormat("%d", luts.size())},
          {"$comma_separated_luts", absl::StrJoin(luts, ", ")},
          {"$run_level_ops", absl::StrJoin(run_level_ops, "\n")},
          {"$output_assignment_block", absl::StrJoin(output_assignments, "\n")},
          {"$return_statement", output_stem},
      });

  auto metadata = CreateFunctionMetadata(
      {{"data", false, /*return_bit_width=*/3}}, true, num_outputs);
  XLS_ASSERT_OK_AND_ASSIGN(
      xls::netlist::AbstractCellLibrary<bool> cell_library,
      fully_homomorphic_encryption::transpiler::ParseCellLibrary(kCells));
  XLS_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<::xls::netlist::rtl::AbstractNetlist<bool>> netlist,
      fully_homomorphic_encryption::transpiler::ParseNetlist(cell_library,
                                                             netlist_text));
  YosysTfheRsTranspiler transpiler(metadata, std::move(netlist));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual, transpiler.Translate());
  EXPECT_EQ(expected, actual);
}

TEST(YosysTfheRsTranspilerTest, ConstantsInOutputAssignment) {
  constexpr absl::string_view netlist_text = R"netlist(
module test_module(data, out);
  wire _0_;
  input [2:0] data;
  wire [2:0] data;
  output [2:0] out;
  wire [2:0] out;
  lut3 _0_ ( .A(data[0]), .B(data[1]), .C(data[2]), .P0(1'h0), .P1(1'h0),
  .P2(1'h0), .P3(1'h1), .P4(1'h1), .P5(1'h1), .P6(1'h1), .P7(1'h0),
  .Y(_0_)); assign out[2:0] = 6'h01;
endmodule
)netlist";

  absl::string_view signature =
      "test_module(data: &Vec<Ciphertext>, server_key: &ServerKey) -> "
      "Vec<Ciphertext>";
  vector<int> luts = {120};
  const int num_outputs = 3;
  const int num_gates = 1;
  const absl::string_view output_stem = "out";
  vector<std::string> ordered_params = {"data"};

  vector<std::string> tasks = {
      "((0, false, LUT3(120)), &[Arg(0, 0), Arg(0, 1), Arg(0, 2)]),"};

  vector<std::string> gate_levels = {
      BuildExpectedGateOps(0, tasks),
  };
  vector<std::string> run_level_ops = {
      absl::StrFormat(kRunLevelTemplate, 0),
  };
  vector<std::string> output_assignments = {
      "    out[0] = constant_true.clone();",
      "    out[1] = constant_false.clone();",
      "    out[2] = constant_false.clone();",
  };
  std::string expected = absl::StrReplaceAll(
      kCodegenTemplate,
      {
          {"$gate_levels", absl::StrJoin(gate_levels, "\n")},
          {"$function_signature", signature},
          {"$ordered_params", absl::StrJoin(ordered_params, ", ")},
          {"$num_luts", absl::StrFormat("%d", luts.size())},
          {"$comma_separated_luts", absl::StrJoin(luts, ", ")},
          {"$total_num_gates", absl::StrFormat("%d", num_gates)},
          {"$output_stem", output_stem},
          {"$num_outputs", absl::StrFormat("%d", num_outputs)},
          {"$run_level_ops", absl::StrJoin(run_level_ops, "\n")},
          {"$output_assignment_block", absl::StrJoin(output_assignments, "\n")},
          {"$return_statement", output_stem},
      });

  auto metadata = CreateFunctionMetadata(
      {{"data", false, /*return_bit_width=*/3}}, true, num_outputs);
  XLS_ASSERT_OK_AND_ASSIGN(
      xls::netlist::AbstractCellLibrary<bool> cell_library,
      fully_homomorphic_encryption::transpiler::ParseCellLibrary(kCells));
  XLS_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<::xls::netlist::rtl::AbstractNetlist<bool>> netlist,
      fully_homomorphic_encryption::transpiler::ParseNetlist(cell_library,
                                                             netlist_text));
  YosysTfheRsTranspiler transpiler(metadata, std::move(netlist));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual, transpiler.Translate());
  EXPECT_EQ(expected, actual);
}

TEST(YosysTfheRsTranspilerTest, OutParamsAndReturnValue) {
  // In this example, the outparam is implicit, and its position in the `out`
  // wireset must be inferred.
  constexpr absl::string_view netlist_text = R"netlist(
module test_module(data, outparam, out);
  wire _0_;
  input [2:0] data;
  wire [2:0] data;
  input [2:0] outparam;
  wire [2:0] outparam;
  output [5:0] out;
  wire [5:0] out;
  lut3 _0_ ( .A(data[0]), .B(data[1]), .C(data[2]), .P0(1'h0), .P1(1'h0),
  .P2(1'h0), .P3(1'h1), .P4(1'h1), .P5(1'h1), .P6(1'h1), .P7(1'h0),
  .Y(out[5])); assign out[4:0] = 6'h01;
endmodule
)netlist";

  absl::string_view signature =
      "test_module(data: &Vec<Ciphertext>, outparam: &mut Vec<Ciphertext>, "
      "server_key: &ServerKey) -> Vec<Ciphertext>";

  vector<int> luts = {120};
  const int num_outputs = 6;
  const int num_gates = 1;
  const absl::string_view output_stem = "out";
  vector<std::string> ordered_params = {"data", "outparam"};

  vector<std::string> tasks = {
      "((5, true, LUT3(120)), &[Arg(0, 0), Arg(0, 1), Arg(0, 2)]),"};

  vector<std::string> gate_levels = {
      BuildExpectedGateOps(0, tasks),
  };
  vector<std::string> run_level_ops = {
      absl::StrFormat(kRunLevelTemplate, 0),
  };
  std::string_view expected_assignments =
      R"rust(    out[0] = constant_true.clone();
    out[1] = constant_false.clone();
    out[2] = constant_false.clone();
    out[3] = constant_false.clone();
    out[4] = constant_false.clone();
    outparam[0] = out[0].clone();
    outparam[1] = out[1].clone();
    outparam[2] = out[2].clone();
    out = out.split_off(3);)rust";
  std::string expected = absl::StrReplaceAll(
      kCodegenTemplate,
      {
          {"$gate_levels", absl::StrJoin(gate_levels, "\n")},
          {"$function_signature", signature},
          {"$ordered_params", absl::StrJoin(ordered_params, ", ")},
          {"$num_luts", absl::StrFormat("%d", luts.size())},
          {"$comma_separated_luts", absl::StrJoin(luts, ", ")},
          {"$total_num_gates", absl::StrFormat("%d", num_gates)},
          {"$output_stem", output_stem},
          {"$num_outputs", absl::StrFormat("%d", num_outputs)},
          {"$run_level_ops", absl::StrJoin(run_level_ops, "\n")},
          {"$output_assignment_block", expected_assignments},
          {"$return_statement", output_stem},
      });

  auto metadata =
      CreateFunctionMetadata({{"data", false, /*return_bit_width=*/3},
                              {"outparam", true, /*return_bit_width=*/3}},
                             true, num_outputs);
  XLS_ASSERT_OK_AND_ASSIGN(
      xls::netlist::AbstractCellLibrary<bool> cell_library,
      fully_homomorphic_encryption::transpiler::ParseCellLibrary(kCells));
  XLS_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<::xls::netlist::rtl::AbstractNetlist<bool>> netlist,
      fully_homomorphic_encryption::transpiler::ParseNetlist(cell_library,
                                                             netlist_text));
  YosysTfheRsTranspiler transpiler(metadata, std::move(netlist));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual, transpiler.Translate());
  EXPECT_EQ(expected, actual);
}

TEST(YosysTfheRsTranspilerTest, ConstParamsNotTreatedAsOutparam) {
  // In this example, the outparam is implicit, and its position in the `out`
  // wireset must be inferred.
  constexpr absl::string_view netlist_text = R"netlist(
module test_module(data, const_ref, out);
  wire _0_;
  input [2:0] data;
  wire [2:0] data;
  input [2:0] const_ref;
  wire [2:0] const_ref;
  output [5:0] out;
  wire [5:0] out;
  lut3 _0_ ( .A(data[0]), .B(data[1]), .C(data[2]), .P0(1'h0), .P1(1'h0),
  .P2(1'h0), .P3(1'h1), .P4(1'h1), .P5(1'h1), .P6(1'h1), .P7(1'h0),
  .Y(out[5])); assign out[4:0] = 6'h01;
endmodule
)netlist";

  absl::string_view signature =
      "test_module(data: &Vec<Ciphertext>, const_ref: &Vec<Ciphertext>, "
      "server_key: &ServerKey) -> Vec<Ciphertext>";

  vector<int> luts = {120};
  const int num_outputs = 6;
  const int num_gates = 1;
  const absl::string_view output_stem = "out";
  vector<std::string> ordered_params = {"data", "const_ref"};

  vector<std::string> tasks = {
      "((5, true, LUT3(120)), &[Arg(0, 0), Arg(0, 1), Arg(0, 2)]),"};

  vector<std::string> gate_levels = {
      BuildExpectedGateOps(0, tasks),
  };
  vector<std::string> run_level_ops = {
      absl::StrFormat(kRunLevelTemplate, 0),
  };
  std::string_view expected_assignments =
      R"rust(    out[0] = constant_true.clone();
    out[1] = constant_false.clone();
    out[2] = constant_false.clone();
    out[3] = constant_false.clone();
    out[4] = constant_false.clone();)rust";
  std::string expected = absl::StrReplaceAll(
      kCodegenTemplate,
      {
          {"$gate_levels", absl::StrJoin(gate_levels, "\n")},
          {"$function_signature", signature},
          {"$ordered_params", absl::StrJoin(ordered_params, ", ")},
          {"$num_luts", absl::StrFormat("%d", luts.size())},
          {"$comma_separated_luts", absl::StrJoin(luts, ", ")},
          {"$total_num_gates", absl::StrFormat("%d", num_gates)},
          {"$output_stem", output_stem},
          {"$num_outputs", absl::StrFormat("%d", num_outputs)},
          {"$run_level_ops", absl::StrJoin(run_level_ops, "\n")},
          {"$output_assignment_block", expected_assignments},
          {"$return_statement", output_stem},
      });

  auto metadata =
      CreateFunctionMetadata({{"data", false, /*return_bit_width=*/3},
                              {"const_ref", false,
                               /*return_bit_width=*/3}},
                             true, num_outputs);
  XLS_ASSERT_OK_AND_ASSIGN(
      xls::netlist::AbstractCellLibrary<bool> cell_library,
      fully_homomorphic_encryption::transpiler::ParseCellLibrary(kCells));
  XLS_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<::xls::netlist::rtl::AbstractNetlist<bool>> netlist,
      fully_homomorphic_encryption::transpiler::ParseNetlist(cell_library,
                                                             netlist_text));
  YosysTfheRsTranspiler transpiler(metadata, std::move(netlist));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual, transpiler.Translate());
  EXPECT_EQ(expected, actual);
}

TEST(YosysTfheRsTranspilerTest, TranslateSimpleBooleanGateFunction) {
  constexpr absl::string_view netlist_text = R"netlist(
module test_module(data, out);
  wire _0_;
  input [2:0] data;
  wire [2:0] data;
  output [1:0] out;
  wire [1:0] out;
  and2 _2_ ( .A(data[0]), .B(data[1]), .Y(_0_));
  assign { out[1:0] } = { data[0], _0_ };
endmodule
)netlist";

  absl::string_view signature =
      "test_module(data: &Vec<Ciphertext>, server_key: &ServerKey) -> "
      "Vec<Ciphertext>";
  const int num_outputs = 2;
  const int num_gates = 1;
  const absl::string_view output_stem = "out";
  vector<std::string> ordered_params = {"data"};
  vector<std::string> outputs = {"temp_nodes[0]", "data[0]"};

  vector<std::string> tasks = {"((0, false, AND2), &[Arg(0, 0), Arg(0, 1)]),"};
  vector<std::string> output_assignments = {
      "    out[0] = temp_nodes[&0].clone();",
      "    out[1] = data[0].clone();",
  };
  std::string expected = absl::StrReplaceAll(
      kCodegenTemplate,
      {
          {"$gate_levels", BuildExpectedGateOps(0, tasks)},
          {"$function_signature", signature},
          {"$ordered_params", absl::StrJoin(ordered_params, ", ")},
          {"$total_num_gates", absl::StrFormat("%d", num_gates)},
          {"$output_stem", output_stem},
          {"$num_outputs", absl::StrFormat("%d", num_outputs)},
          {"$num_luts", absl::StrFormat("%d", 0)},
          {"$comma_separated_luts", ""},
          {"$run_level_ops", absl::StrFormat(kRunLevelTemplate, 0)},
          {"$output_assignment_block", absl::StrJoin(output_assignments, "\n")},
          {"$return_statement", output_stem},
      });

  auto metadata = CreateFunctionMetadata(
      {{"data", false, /*return_bit_width=*/3}}, true, num_outputs);
  XLS_ASSERT_OK_AND_ASSIGN(
      xls::netlist::AbstractCellLibrary<bool> cell_library,
      fully_homomorphic_encryption::transpiler::ParseCellLibrary(kCells));
  XLS_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<::xls::netlist::rtl::AbstractNetlist<bool>> netlist,
      fully_homomorphic_encryption::transpiler::ParseNetlist(cell_library,
                                                             netlist_text));
  YosysTfheRsTranspiler transpiler(metadata, std::move(netlist));

  XLS_ASSERT_OK_AND_ASSIGN(std::string actual, transpiler.Translate());
  EXPECT_EQ(expected, actual);
}

}  // namespace
}  // namespace transpiler
}  // namespace rust
}  // namespace fhe
