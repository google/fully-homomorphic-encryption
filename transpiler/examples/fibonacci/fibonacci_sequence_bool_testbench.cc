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
#include "transpiler/data/boolean_data.h"
#include "transpiler/examples/fibonacci/fibonacci_sequence_bool.h"
#include "xls/common/logging/logging.h"

int main(int argc, char** argv) {
  EncodedInt input(5);
  EncodedArray<int> encoded_result({0, 0, 0, 0, 0});

  XLS_CHECK_OK(fibonacci_sequence(input.get(), encoded_result.get()));
  absl::FixedArray<int> result = encoded_result.Decode();

  std::cout << absl::StrFormat("Result: %d, %d, %d, %d, %d", result[0],
                               result[1], result[2], result[3], result[4])
            << std::endl;

  return 0;
}
