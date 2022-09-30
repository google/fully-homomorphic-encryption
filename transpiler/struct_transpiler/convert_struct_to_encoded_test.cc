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

#include "transpiler/struct_transpiler/convert_struct_to_encoded.h"

#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "xls/common/status/matchers.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"

namespace fully_homomorphic_encryption::transpiler {
namespace {

using testing::HasSubstr;

// Verifies that an int-returning (and no-arg-taking) function generates a
// correct header.
TEST(ConvertStructsToEncodedTest, ReturnsInt) {
  constexpr const char kMetadataStr[] = R"(
structs {
  as_struct {
    name {
      as_inst {
        name {
          name: "TwoBytesAndAShort"
          fully_qualified_name: "some_namespace::TwoBytesAndAShort"
        }
      }
    }
    fields {
      name: "first"
      type {
        as_int {
          width: 8
          is_signed: false
        }
      }
    }
    fields {
      name: "second"
      type {
        as_int {
          width: 8
          is_signed: false
        }
      }
    }
    fields {
      name: "third"
      type {
        as_int {
          width: 16
          is_signed: false
        }
      }
    }
  }
}
top_func_proto {
  name {
    name: "ReturnsInt"
    fully_qualified_name: "foo::ReturnsInt"
    id: 0
  }
  return_type {
    as_int {
      width: 32
      is_signed: false
    }
  }
}
  )";
  xlscc_metadata::MetadataOutput metadata;
  google::protobuf::TextFormat::ParseFromString(kMetadataStr, &metadata);
  XLS_ASSERT_OK_AND_ASSIGN(
      std::string actual,
      ConvertStructsToEncodedTemplate(metadata, /*original_headers=*/{},
                                      /*output_path=*/"", /*unwrap=*/{}));

  // Rather than do line-by-line equality checks, let's just make sure that a
  // few key lines are present. Since we're generating compilable code,
  // integration tests are a good place to focus effort (see "examples/structs"
  // for those).
  ASSERT_THAT(actual, HasSubstr("TwoBytesAndAShort"));
  ASSERT_THAT(actual, HasSubstr("some_namespace::TwoBytesAndAShort"));
  ASSERT_THAT(actual, HasSubstr("result->first = encoded_first.Decode()"));
  ASSERT_THAT(actual, HasSubstr("result->second = encoded_second.Decode()"));
  ASSERT_THAT(actual, HasSubstr("result->third = encoded_third.Decode()"));
  ASSERT_THAT(actual,
              HasSubstr("UnencryptedFn(EncodedValue<uint8_t>(value."
                        "first).get(), key, absl::MakeSpan(data, 8));"));
  ASSERT_THAT(actual, HasSubstr("EncryptFn(EncodedValue<uint8_t>(value.second)."
                                "get(), key, absl::MakeSpan(data, 8));"));
  ASSERT_THAT(actual, HasSubstr("DecryptFn(absl::MakeConstSpan(data, 16), key, "
                                "encoded_third.get());"));
}

}  // namespace

}  // namespace fully_homomorphic_encryption::transpiler
