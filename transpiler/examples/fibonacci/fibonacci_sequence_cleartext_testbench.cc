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

#include <array>
#include <iostream>
#include <string>
#include <vector>

#include "absl/container/fixed_array.h"
#include "absl/strings/str_format.h"
#include "transpiler/data/cleartext_data.h"
#include "transpiler/examples/fibonacci/fibonacci_sequence.h"
#include "transpiler/examples/fibonacci/fibonacci_sequence_cleartext.h"
#include "xls/common/logging/logging.h"

int main(int argc, char** argv) {
  Encoded<int> input(7);
  EncodedArray<int, FIBONACCI_SEQUENCE_SIZE> encoded_result({0, 0, 0, 0, 0});

  XLS_CHECK_OK(fibonacci_sequence(input, encoded_result));

  absl::FixedArray<int> result = encoded_result.Decode();
  for (int i = 0; i < result.size(); i++) {
    std::cout << absl::StrFormat("Result %d: %d", i, result[i]) << std::endl;
  }

  return 0;
}
