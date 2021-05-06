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

#include "pir_api.h"

namespace fully_homomorphic_encryption {

#pragma hls_top
RecordT QueryRecord(Index index, const RecordT database[kDbSize]) {
  // Performs a "select" operation on all records in `database` corresponding
  // to some key, `index`. Both `index` and `database` are encrypted under the
  // user's private key. This is an O(N) operation, where N is the number of
  // records in `database`.
  //
  // See more at: //transpiler/README.oss.md.
  return database[index];
}

}  // namespace fully_homomorphic_encryption
