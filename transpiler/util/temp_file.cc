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

#include "transpiler/util/temp_file.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

namespace fully_homomorphic_encryption {
namespace transpiler {

/*static*/ absl::StatusOr<TempFile> TempFile::Create() {
  // Modified in mkostemp.
  std::string templ = "/tmp/fhe_temp_XXXXXX";
  int fd = mkostemp(templ.data(), O_CLOEXEC);
  if (fd == -1) {
    return absl::UnavailableError(
        absl::StrCat("Failed to create temporary file ", templ.data(), ": ",
                     strerror(errno)));
  }
  close(fd);
  return TempFile(templ);
}

TempFile::~TempFile() { Cleanup(); }

void TempFile::Cleanup() {
  if (!path_.empty()) {
    if (unlink(path_.c_str()) != 0) {
      std::cerr << "Unable to delete temp file: " << path_.string()
                << std::endl;
    }
    path_.clear();
  }
}

TempFile::TempFile(TempFile&& other) : path_(std::move(other.path_)) {
  other.path_.clear();
}

TempFile& TempFile::operator=(TempFile&& other) {
  Cleanup();
  path_ = std::move(other.path_);
  other.path_.clear();
  return *this;
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
