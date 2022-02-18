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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TEMP_FILE_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TEMP_FILE_H_

#include <filesystem>

#include "absl/status/statusor.h"

namespace fully_homomorphic_encryption::transpiler {

// A simple delete-on-destroy temporary file holder.
class TempFile {
 public:
  static absl::StatusOr<TempFile> Create();
  ~TempFile();

  // Moveable but not copyable.
  TempFile(TempFile&& other);
  TempFile& operator=(TempFile&& other);
  TempFile(const TempFile&) = delete;
  TempFile& operator=(const TempFile&) = delete;

  std::filesystem::path path() { return path_; }

 private:
  TempFile(const std::filesystem::path& path) : path_(path) {}
  void Cleanup();
  std::filesystem::path path_;
};

}  // namespace fully_homomorphic_encryption::transpiler

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_TEMP_FILE_H_
