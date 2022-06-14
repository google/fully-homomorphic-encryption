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

#include "transpiler/examples/pir/pir_cloud_service.h"

#include "absl/status/status.h"
#include "absl/types/span.h"

#ifdef USE_INTERPRETED_TFHE
#include "transpiler/examples/pir/pir_api_interpreted_tfhe.h"
#else
#include "transpiler/examples/pir/pir_api_tfhe.h"
#endif

namespace fully_homomorphic_encryption {

absl::Status CloudService::QueryRecord(
    TfheRef<unsigned char> result, TfheRef<unsigned char> index,
    const TFheGateBootstrappingCloudKeySet* bk) {
  return ::QueryRecord(result, index, database_, bk);
}

}  // namespace fully_homomorphic_encryption
