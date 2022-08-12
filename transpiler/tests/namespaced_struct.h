// Copyright 2022 Google LLC
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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TESTS_NAMESPACED_STRUCT_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TESTS_NAMESPACED_STRUCT_H_

namespace outer {
namespace inner {
constexpr int kSumSimpleArraySize = 3;

// A struct nested within a namespace, to ensure that the transpiler properly
// leverages fully qualified type names when generating signatures.
struct Simple {
  unsigned char value;
};

unsigned short sum_simple_structs(Simple data[kSumSimpleArraySize]);
}  // namespace inner
}  // namespace outer

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TESTS_NAMESPACED_STRUCT_H_
