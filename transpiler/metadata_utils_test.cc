// Copyright 2021 Google LLC
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
#include "transpiler/metadata_utils.h"

#include <memory>
#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/netlist_utils.h"
#include "xls/common/status/matchers.h"
#include "xls/common/status/status_macros.h"
#include "xls/netlist/netlist.h"

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

using ::xls::netlist::AbstractCellLibrary;
using ::xls::netlist::rtl::AbstractNetlist;
using ::xls::status_testing::IsOk;
using ::xls::status_testing::StatusIs;

constexpr absl::string_view kCells = R"lib(
library(testlib) {
  cell(and2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A * B)";}}
  cell(or2) {pin(A) {direction : input;} pin(B) {direction : input;} pin(Y) {direction: output; function : "(A + B)";}}
}
)lib";

absl::StatusOr<xlscc_metadata::MetadataOutput> run_create_metadata(
    absl::string_view netlist, absl::string_view metadata) {
  XLS_ASSIGN_OR_RETURN(AbstractCellLibrary<bool> cell_library,
                       ParseCellLibrary(std::string(kCells)));
  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<AbstractNetlist<bool>> parsed_netlist_ptr,
      ParseNetlist(cell_library, netlist));

  return CreateMetadataFromHeirJson(metadata,
                                    parsed_netlist_ptr->modules().front());
}

TEST(CreateMetadataFromJsonTest, NoMainFunction) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [1:0] in;
  wire [1:0] in;
  output [1:0] out;
  wire [1:0] out;
  and2 _3_ ( .A(in[1]), .B(_0_), .Y(_1_));
  or2 _2_ ( .A(in[0]), .B(in[1]), .Y(_0_));
  assign { out[1:0] } = { _1_, _0_ };
endmodule
)netlist";

  static constexpr absl::string_view metadata = R"({
"functions": [{
  "name": "test",
  "params": [
    {
      "index": 0,
      "type": {
        "integer": {
          "is_signed": false,
          "width": 10
        }
      }
    }
  ],
  "return_type": {
    "integer": {
      "is_signed": true,
      "width": 2
    }
  }
}]
})";

  absl::StatusOr<xlscc_metadata::MetadataOutput> output =
      run_create_metadata(netlist, metadata);
  EXPECT_THAT(output, StatusIs(absl::StatusCode::kInvalidArgument,
                               testing::HasSubstr("expected main function")));
}

TEST(CreateMetadataFromJsonTest, InvalidType) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [1:0] in;
  wire [1:0] in;
  output [1:0] out;
  wire [1:0] out;
  and2 _3_ ( .A(in[1]), .B(_0_), .Y(_1_));
  or2 _2_ ( .A(in[0]), .B(in[1]), .Y(_0_));
  assign { out[1:0] } = { _1_, _0_ };
endmodule
)netlist";

  static constexpr absl::string_view metadata = R"({
"functions": [{
  "name": "main",
  "params": [
    {
      "index": 0,
      "type": {
        "other": {
          "is_signed": false,
          "width": 10
        }
      }
    }
  ],
  "return_type": {
    "other": {
      "is_signed": true,
      "width": 2
    }
  }
}]
})";

  absl::StatusOr<xlscc_metadata::MetadataOutput> output =
      run_create_metadata(netlist, metadata);
  EXPECT_THAT(output, StatusIs(absl::StatusCode::kInvalidArgument,
                               testing::HasSubstr("unexpected type")));
}

TEST(CreateMetadataFromJsonTest, SimpleIntTypes) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [1:0] in;
  wire [1:0] in;
  output [1:0] out;
  wire [1:0] out;
  and2 _3_ ( .A(in[1]), .B(_0_), .Y(_1_));
  or2 _2_ ( .A(in[0]), .B(in[1]), .Y(_0_));
  assign { out[1:0] } = { _1_, _0_ };
endmodule
)netlist";

  static constexpr absl::string_view metadata = R"({
"functions": [{
  "name": "main",
  "params": [
    {
      "index": 0,
      "type": {
        "integer": {
          "is_signed": false,
          "width": 10
        }
      }
    }
  ],
  "return_type": {
    "integer": {
      "is_signed": true,
      "width": 2
    }
  }
}]
})";

  absl::StatusOr<xlscc_metadata::MetadataOutput> output =
      run_create_metadata(netlist, metadata);
  EXPECT_THAT(output, IsOk());

  EXPECT_EQ(output->top_func_proto().params(0).type().as_int().width(), 10);
  EXPECT_EQ(output->top_func_proto().params(0).type().as_int().is_signed(),
            false);
  EXPECT_EQ(output->top_func_proto().return_type().as_int().width(), 2);
  EXPECT_EQ(output->top_func_proto().return_type().as_int().is_signed(), true);
}

TEST(CreateMetadataFromJsonTest, MemRefTypes) {
  static constexpr absl::string_view netlist = R"netlist(
module TestModule(in, out);
  wire _0_;
  wire _1_;
  input [31:0] in;
  wire [31:0] in;
  output [31:0] out;
  wire [31:0] out;
  assign { out[31:0] } = 32'b0;
endmodule
)netlist";

  static constexpr absl::string_view metadata = R"({
"functions": [{
  "name": "main",
  "params": [
    {
      "index": 0,
      "type": {
        "memref": {
          "element_type": {
            "integer": {
              "is_signed": false,
              "width": 8
            }
          },
          "shape": [
            1,
            4
          ]
        }
      }
    }
  ],
  "return_type": {
    "memref": {
      "element_type": {
        "integer": {
          "is_signed": false,
          "width": 8
        }
      },
      "shape": [
        1,
        4
      ]
    }
  }
}]
})";

  absl::StatusOr<xlscc_metadata::MetadataOutput> output =
      run_create_metadata(netlist, metadata);
  EXPECT_THAT(output, IsOk());

  for (auto type : std::vector<xlscc_metadata::Type>{
           output->top_func_proto().params(0).type(),
           output->top_func_proto().return_type()}) {
    EXPECT_TRUE(type.has_as_array());
    EXPECT_EQ(type.as_array().size(), 1);
    EXPECT_TRUE(type.as_array().element_type().has_as_array());
    auto inner_array = type.as_array().element_type().as_array();
    EXPECT_EQ(inner_array.size(), 4);
    EXPECT_TRUE(inner_array.element_type().has_as_int());
    EXPECT_EQ(inner_array.element_type().as_int().width(), 8);
    EXPECT_EQ(inner_array.element_type().as_int().is_signed(), false);
  }
}

}  // namespace
}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
