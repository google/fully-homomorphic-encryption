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

#ifdef USE_INTERPRETED_TFHE
#include "transpiler/examples/pir/pir_api_interpreted_tfhe.h"
#else
#include "transpiler/examples/pir/pir_api_tfhe.h"
#endif

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/data/tfhe_data.h"
#include "transpiler/examples/pir/pir_api.h"
#include "transpiler/tests/tfhe_test_util.h"
#include "xls/common/status/matchers.h"

namespace fully_homomorphic_encryption {
namespace {

using TranspilerTestBase =
    ::fully_homomorphic_encryption::transpiler::TranspilerTestBase;
using ::testing::TestParamInfo;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

using TfheIndex = Tfhe<unsigned char>;
using TfheRecordT = Tfhe<unsigned char>;

struct TranspilerExamplesPirTestCase {
  const std::string test_name;
  const Index index;
  const std::vector<RecordT> db;
  const RecordT expected_output;
};

class TranspilerExamplesPirTest
    : public TranspilerTestBase,
      public WithParamInterface<TranspilerExamplesPirTestCase> {};

TEST_P(TranspilerExamplesPirTest, TestPir) {
  const TranspilerExamplesPirTestCase& test_case = GetParam();

  auto index_ciphertext = TfheIndex::Encrypt(test_case.index, secret_key());
  auto db_ciphertext = TfheArray<RecordT>::Encrypt(test_case.db, secret_key());
  TfheRecordT result_ciphertext(params());

  XLS_ASSERT_OK(QueryRecord(result_ciphertext, index_ciphertext, db_ciphertext,
                            cloud_key()));
  EXPECT_EQ(result_ciphertext.Decrypt(secret_key()), test_case.expected_output);
}

INSTANTIATE_TEST_SUITE_P(
    TranspilerExamplesPirTests, TranspilerExamplesPirTest,
    ValuesIn<TranspilerExamplesPirTestCase>({
        {"RetrieveFirstRecord", 0, {'a', 'b', 'c'}, 'a'},
        {"RetrieveSecondRecord", 1, {'x', 'y', 'z'}, 'y'},
        {"RetrieveThirdRecord", 2, {'1', '2', '3'}, '3'},
    }),
    [](const TestParamInfo<TranspilerExamplesPirTest::ParamType>& info) {
      return info.param.test_name;
    });

}  // namespace
}  // namespace fully_homomorphic_encryption
