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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/data/cleartext_data.h"
#include "transpiler/tests/out_params_cleartext.h"
#include "transpiler/tests/out_params_with_return_cleartext.h"
#include "transpiler/tests/single_out_param_cleartext.h"
#include "transpiler/tests/single_out_param_with_return_cleartext.h"
#include "xls/common/status/matchers.h"

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

TEST(OutParamsTest, TwoOutParams) {
  auto x = Encoded<int>(10);
  auto y = Encoded<int>(20);
  XLS_ASSERT_OK(out_params(x, y));
  EXPECT_EQ(x.Decode(), 10);
  EXPECT_EQ(y.Decode(), 11);
}

TEST(OutParamsTest, TwoOutParamsWithReturn) {
  auto x = Encoded<int>(10);
  auto y = Encoded<int>(20);
  Encoded<int> result;
  XLS_ASSERT_OK(out_params_with_return(result, x, y));
  EXPECT_EQ(result.Decode(), 31);
  EXPECT_EQ(x.Decode(), 10);
  EXPECT_EQ(y.Decode(), 11);
}

TEST(OutParamsTest, OneOutParam) {
  auto x = Encoded<int>(10);
  XLS_ASSERT_OK(single_out_param(x));
  EXPECT_EQ(x.Decode(), 11);
}

TEST(OutParamsTest, OneOutParamWithReturn) {
  auto x = Encoded<int>(10);
  Encoded<int> result;
  XLS_ASSERT_OK(single_out_param_with_return(result, x));
  EXPECT_EQ(result.Decode(), 27);
  EXPECT_EQ(x.Decode(), 11);
}

}  // namespace
}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
