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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_TRANSPILER_EXAMPLES_PIR_API_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_TRANSPILER_EXAMPLES_PIR_API_H_

// The supported "cloud database" size. Must be known at compile-time for FHE.
// Should be a value between [0, 255].
// TODO: Replace with uint8_t.
constexpr unsigned char kDbSize = 3;

namespace fully_homomorphic_encryption {

// TODO: Replace with uint8_t.
using Index = unsigned char;
using RecordT = unsigned char;

// Returns a record of interest from an underlying "database" using FHE.
RecordT QueryRecord(Index index, const RecordT database[kDbSize]);

}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_TRANSPILER_EXAMPLES_PIR_API_H_
