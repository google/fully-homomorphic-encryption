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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_TRANSPILER_EXAMPLES_PIR_CLOUD_SERVICE_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_TRANSPILER_EXAMPLES_PIR_CLOUD_SERVICE_H_

#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
#include "transpiler/data/tfhe_data.h"
#include "transpiler/examples/pir/pir_api.h"

namespace fully_homomorphic_encryption {

// Emulates a basic "CloudService" connection which allows for queries to hosted
// data. Both the queries and the data are encrypted under the client's private
// key.
class CloudService {
 public:
  explicit CloudService(TfheArray<RecordT, kDbSize> database)
      : database_(std::move(database)) {}

  absl::Status QueryRecord(TfheRef<unsigned char> result,
                           TfheRef<unsigned char> index,
                           const TFheGateBootstrappingCloudKeySet* bk);

 private:
  TfheArray<RecordT, kDbSize> database_;
};

}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_TRANSPILER_EXAMPLES_PIR_CLOUD_SERVICE_H_
