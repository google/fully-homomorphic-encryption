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

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "transpiler/struct_transpiler/convert_struct_to_encoded.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"

ABSL_FLAG(std::string, metadata_path, "",
          "Path to the FHE function metadata [binary] proto.");
ABSL_FLAG(std::string, original_headers, "",
          "Path to the original header file{s} being transpiled. "
          "Comma-separated if more than one.");
ABSL_FLAG(std::string, output_path, "",
          "Path to which to write the generated file. "
          "If unspecified, output will be written to stdout.");

namespace fully_homomorphic_encryption {
namespace transpiler {

absl::Status RealMain(absl::string_view metadata_path,
                      absl::string_view original_headers,
                      absl::string_view output_path) {
  XLS_ASSIGN_OR_RETURN(std::string proto_data,
                       xls::GetFileContents(metadata_path));
  xlscc_metadata::MetadataOutput metadata;
  if (!metadata.ParseFromString(proto_data)) {
    return absl::InvalidArgumentError("Unable to parse input metadata proto.");
  }

  std::vector<std::string> split_headers =
      absl::StrSplit(original_headers, ',');
  XLS_ASSIGN_OR_RETURN(std::string result,
                       ConvertStructsToEncoded(metadata, split_headers));
  if (!output_path.empty()) {
    return xls::SetFileContents(output_path, result);
  }

  std::cout << result << std::endl;
  return absl::OkStatus();
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage(argv[0]);
  absl::ParseCommandLine(argc, argv);

  std::string metadata_path = absl::GetFlag(FLAGS_metadata_path);
  if (metadata_path.empty()) {
    std::cout << "--metadata_path cannot be empty." << std::endl;
    return 1;
  }
  std::string original_headers = absl::GetFlag(FLAGS_original_headers);
  std::string output_path = absl::GetFlag(FLAGS_output_path);

  absl::Status status = fully_homomorphic_encryption::transpiler::RealMain(
      metadata_path, original_headers, output_path);
  if (!status.ok()) {
    std::cerr << status.message() << std::endl;
    return 1;
  }
  return 0;
}
