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

#include "transpiler/util/runfiles.h"

#include <unistd.h>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"

namespace fully_homomorphic_encryption {

// This is pretty unwieldy, but it suffices for now. It's a prime candidate for
// cleanups, though!
absl::StatusOr<std::filesystem::path> GetRunfilePath(
    const std::filesystem::path& leaf, absl::optional<std::string> package) {
  char* real_path = realpath("/proc/self/exe", NULL);
  std::filesystem::path runfile_path = real_path;
  runfile_path += ".runfiles";

  std::vector<std::filesystem::path> base_paths = {real_path, runfile_path,
                                                   "."};
  free(real_path);
  for (const auto& base_path : base_paths) {
    std::string full_path = base_path / leaf;
    if (!access(full_path.data(), F_OK)) {
      return full_path;
    }

    std::vector<std::string> elements = {

    };

    if (package.has_value()) {
      elements.push_back(package.value());
    }

    for (const auto& element : elements) {
      full_path = base_path / element / leaf;
      if (!access(full_path.data(), F_OK)) {
        return full_path;
      }
    }
  }

  return absl::NotFoundError(absl::StrCat(
      "Could not find a path to \"", static_cast<std::string>(leaf), "\""));
}

}  // namespace fully_homomorphic_encryption
