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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_UTIL_SUBPROCESS_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_UTIL_SUBPROCESS_H_

#include <filesystem>
#include <string>

#include "absl/status/statusor.h"
#include "absl/types/span.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

// Runs the subprocess/args given by argv (argv[0] being the path to the
// executable) and returns a pair of strings holding that process' stdout and
// stderr, respectively.
absl::StatusOr<std::pair<std::string, std::string>> InvokeSubprocess(
    absl::Span<const std::string> argv, const std::filesystem::path& cwd = "");

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_UTIL_SUBPROCESS_H_
