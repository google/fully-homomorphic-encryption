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

#include "transpiler/tfhe_runner.h"

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/data/tfhe_data.h"
#include "xls/common/status/matchers.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/public/ir_parser.h"

constexpr int kMainMinimumLambda = 120;

using fully_homomorphic_encryption::transpiler::TfheRunner;

// Increments the argument char.
constexpr absl::string_view kEndToEndExample = R"(
package my_package

fn my_package(x: bits[8]) -> bits[8] {
  bit_slice.266: bits[1] = bit_slice(x, start=7, width=1, id=266)
  bit_slice.265: bits[1] = bit_slice(x, start=6, width=1, id=265)
  bit_slice.264: bits[1] = bit_slice(x, start=5, width=1, id=264)
  and.273: bits[1] = and(bit_slice.266, bit_slice.265, id=273)
  and.267: bits[1] = and(bit_slice.265, bit_slice.264, id=267)
  bit_slice.263: bits[1] = bit_slice(x, start=4, width=1, id=263)
  and.274: bits[1] = and(and.273, bit_slice.264, id=274)
  and.268: bits[1] = and(and.267, bit_slice.263, id=268)
  bit_slice.262: bits[1] = bit_slice(x, start=3, width=1, id=262)
  and.275: bits[1] = and(and.274, bit_slice.263, id=275)
  and.280: bits[1] = and(bit_slice.264, bit_slice.263, id=280)
  and.269: bits[1] = and(and.268, bit_slice.262, id=269)
  bit_slice.261: bits[1] = bit_slice(x, start=2, width=1, id=261)
  and.276: bits[1] = and(and.275, bit_slice.262, id=276)
  and.281: bits[1] = and(and.280, bit_slice.262, id=281)
  and.285: bits[1] = and(bit_slice.263, bit_slice.262, id=285)
  and.270: bits[1] = and(and.269, bit_slice.261, id=270)
  bit_slice.260: bits[1] = bit_slice(x, start=1, width=1, id=260)
  and.277: bits[1] = and(and.276, bit_slice.261, id=277)
  and.282: bits[1] = and(and.281, bit_slice.261, id=282)
  and.286: bits[1] = and(and.285, bit_slice.261, id=286)
  and.289: bits[1] = and(bit_slice.262, bit_slice.261, id=289)
  and.271: bits[1] = and(and.270, bit_slice.260, id=271)
  bit_slice.259: bits[1] = bit_slice(x, start=0, width=1, id=259)
  and.278: bits[1] = and(and.277, bit_slice.260, id=278)
  and.283: bits[1] = and(and.282, bit_slice.260, id=283)
  and.287: bits[1] = and(and.286, bit_slice.260, id=287)
  and.290: bits[1] = and(and.289, bit_slice.260, id=290)
  and.292: bits[1] = and(bit_slice.261, bit_slice.260, id=292)
  and.306: bits[1] = and(bit_slice.261, bit_slice.260, id=306)
  and.272: bits[1] = and(and.271, bit_slice.259, id=272)
  and.279: bits[1] = and(and.278, bit_slice.259, id=279)
  and.284: bits[1] = and(and.283, bit_slice.259, id=284)
  and.288: bits[1] = and(and.287, bit_slice.259, id=288)
  and.291: bits[1] = and(and.290, bit_slice.259, id=291)
  and.293: bits[1] = and(and.292, bit_slice.259, id=293)
  and.294: bits[1] = and(bit_slice.260, bit_slice.259, id=294)
  and.307: bits[1] = and(and.306, bit_slice.259, id=307)
  and.316: bits[1] = and(bit_slice.260, bit_slice.259, id=316)
  or.295: bits[1] = or(bit_slice.266, and.272, id=295)
  not.296: bits[1] = not(and.279, id=296)
  or.297: bits[1] = or(bit_slice.265, and.284, id=297)
  not.298: bits[1] = not(and.272, id=298)
  or.299: bits[1] = or(bit_slice.264, and.288, id=299)
  not.300: bits[1] = not(and.284, id=300)
  or.301: bits[1] = or(bit_slice.263, and.291, id=301)
  not.302: bits[1] = not(and.288, id=302)
  or.303: bits[1] = or(bit_slice.262, and.293, id=303)
  not.304: bits[1] = not(and.291, id=304)
  or.305: bits[1] = or(bit_slice.261, and.294, id=305)
  not.308: bits[1] = not(and.307, id=308)
  or.315: bits[1] = or(bit_slice.260, bit_slice.259, id=315)
  not.317: bits[1] = not(and.316, id=317)
  and.309: bits[1] = and(or.295, not.296, id=309)
  and.310: bits[1] = and(or.297, not.298, id=310)
  and.311: bits[1] = and(or.299, not.300, id=311)
  and.312: bits[1] = and(or.301, not.302, id=312)
  and.313: bits[1] = and(or.303, not.304, id=313)
  and.314: bits[1] = and(or.305, not.308, id=314)
  and.318: bits[1] = and(or.315, not.317, id=318)
  not.319: bits[1] = not(bit_slice.259, id=319)
  literal.256: bits[1] = literal(value=1, id=256)
  literal.257: bits[1] = literal(value=0, id=257)
  ret concat.320: bits[8] = concat(and.309, and.310, and.311, and.312, and.313, and.314, and.318, not.319, id=320)
}
)";

TEST(TfheRunnerTest, EndToEnd) {
  TFHEParameters params(kMainMinimumLambda);
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  auto ciphertext = TfheValue<char>::Encrypt('a', key);
  TfheValue<char> result(key.params());
  absl::flat_hash_map<std::string, absl::Span<const LweSample>> in_args = {
      {"x", ciphertext.get()}};

  XLS_ASSERT_OK_AND_ASSIGN(
      auto package,
      xls::ParsePackage(kEndToEndExample, /*filename=*/std::nullopt));
  xlscc_metadata::MetadataOutput metadata;
  metadata.mutable_top_func_proto()->mutable_name()->set_name("my_package");

  TfheRunner runner{std::move(package), metadata};
  XLS_CHECK(runner.Run(result.get(), in_args, {}, key.cloud()).ok());
  auto r = result.Decrypt(key);
  EXPECT_EQ(r, 'b');
}
