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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_UTIL_RUNFILES_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_UTIL_RUNFILES_H_

#include <filesystem>

#include "absl/status/statusor.h"

namespace fully_homomorphic_encryption {

// Returns the full path to the given dependency [path].
// "package" is an optional package prefix for the given dependency, e.g.,
// "com_google_xls".
// TODO: There must be a better way to find dependencies than this.
absl::StatusOr<std::filesystem::path> GetRunfilePath(
    const std::filesystem::path& leaf, absl::optional<std::string> package);

}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_UTIL_RUNFILES_H_
