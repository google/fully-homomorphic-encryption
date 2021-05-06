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

// Takes an booleanified xls ir file and produces an FHE C++ file that uses
// TFHE api.
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "transpiler/cc_transpiler.h"
#include "transpiler/interpreted_tfhe_transpiler.h"
#include "transpiler/tfhe_transpiler.h"
#include "transpiler/util/subprocess.h"
#include "transpiler/util/temp_file.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/ir/function.h"
#include "xls/ir/ir_parser.h"
#include "xls/ir/package.h"

ABSL_FLAG(std::string, booleanify_main_path, "",
          "Path to the XLS IR booleanifier binary.");
ABSL_FLAG(std::string, opt_main_path, "",
          "Path to the XLS IR optimizer binary. "
          "Not needed if --opt_passes is 0.");
ABSL_FLAG(std::string, ir_path, "", "Path to the XLS IR to process.");
ABSL_FLAG(std::string, metadata_path, "",
          "Path to a [binary-format] xlscc MetadataOutput protobuf "
          "containing data about the function to transpile.");
ABSL_FLAG(std::string, output_ir_path, "",
          "Path to place the processed XLS IR (booleanified and, if requested, "
          "optimized).");
ABSL_FLAG(std::string, header_path, "-",
          "Path to generate the C++ header file. If unspecified, output to "
          "stdout");
ABSL_FLAG(std::string, cc_path, "-",
          "Path to generate the C++ source file. If unspecified, output to "
          "stdout after the header file.");
ABSL_FLAG(int, opt_passes, 2, "Number of optimization passes to run.");
ABSL_FLAG(std::string, transpiler_type, "tfhe",
          "Sets the transpiler type; must be one of {tfhe, interpreted_tfhe, "
          "bool}. 'bool' uses native Boolean operations on plaintext rather "
          "than an FHE library, so is mostly useful for debugging.");

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

// This is _definitely_ oversimplified for many purposes, but it seems to
// suffice for right here and right now.
std::filesystem::path GetRunfilePath(const std::filesystem::path& base) {
  return std::filesystem::path(getcwd(NULL, 0)) / base;
}

absl::Status OptimizeAndBooleanify(
    int opt_passes, const std::string& function_name,
    const std::filesystem::path& input_ir_path,
    const std::filesystem::path& booleanifier_path,
    const std::filesystem::path& optimizer_path,
    const std::filesystem::path& booleanized_ir_path) {
  if (opt_passes == 0) {
    XLS_ASSIGN_OR_RETURN(
        auto out_err,
        InvokeSubprocess(
            {GetRunfilePath(booleanifier_path),
             absl::StrCat("--ir_path=",
                          static_cast<std::string>(input_ir_path)),
             absl::StrCat("--function=", function_name),
             absl::StrCat("--output_function_name=", function_name)}));
    XLS_RETURN_IF_ERROR(
        xls::SetFileContents(booleanized_ir_path, std::get<0>(out_err)));
  } else {
    XLS_ASSIGN_OR_RETURN(TempFile optimized, TempFile::Create());
    for (int i = 0; i < opt_passes; i++) {
      XLS_ASSIGN_OR_RETURN(
          auto out_err,
          InvokeSubprocess({GetRunfilePath(optimizer_path),
                            i == 0 ? input_ir_path : booleanized_ir_path}));
      XLS_RETURN_IF_ERROR(
          xls::SetFileContents(optimized.path(), std::get<0>(out_err)));

      XLS_ASSIGN_OR_RETURN(
          out_err,
          InvokeSubprocess(
              {GetRunfilePath(booleanifier_path),
               absl::StrCat("--ir_path=",
                            static_cast<std::string>(optimized.path())),
               absl::StrCat("--function=", function_name),
               absl::StrCat("--output_function_name=", function_name)}));

      XLS_RETURN_IF_ERROR(
          xls::SetFileContents(booleanized_ir_path, std::get<0>(out_err)));
    }
  }

  return absl::OkStatus();
}

}  // namespace

absl::Status RealMain(const std::filesystem::path& ir_path,
                      absl::optional<std::filesystem::path> output_ir_path,
                      const std::filesystem::path& header_path,
                      const std::filesystem::path& cc_path,
                      const std::filesystem::path& booleanify_main_path,
                      const std::filesystem::path& opt_main_path,
                      const std::filesystem::path& metadata_path,
                      int opt_passes) {
  XLS_ASSIGN_OR_RETURN(std::string proto_text,
                       xls::GetFileContents(metadata_path));
  xlscc_metadata::MetadataOutput metadata;
  if (!metadata.ParseFromString(proto_text)) {
    return absl::InvalidArgumentError(
        "Could not parse function metadata proto.");
  }
  const std::string& function_name = metadata.top_func_proto().name().name();

  // Opt/booleanify loop!
  std::filesystem::path booleanized_path;
  absl::optional<TempFile> temp_booleanized;
  if (output_ir_path.has_value()) {
    booleanized_path = output_ir_path.value();
  } else {
    XLS_ASSIGN_OR_RETURN(temp_booleanized, TempFile::Create());
    booleanized_path = temp_booleanized->path();
  }
  XLS_RETURN_IF_ERROR(OptimizeAndBooleanify(opt_passes, function_name, ir_path,
                                            booleanify_main_path, opt_main_path,
                                            booleanized_path));

  XLS_ASSIGN_OR_RETURN(std::string ir_text,
                       xls::GetFileContents(booleanized_path));
  XLS_ASSIGN_OR_RETURN(auto package, xls::Parser::ParsePackage(ir_text));
  XLS_ASSIGN_OR_RETURN(xls::Function * function,
                       package->GetFunction(function_name));

  std::string fn_body, fn_header;
  std::string transpiler_type = absl::GetFlag(FLAGS_transpiler_type);
  if (transpiler_type == "bool") {
    XLS_ASSIGN_OR_RETURN(fn_body, CcTranspiler::Translate(function, metadata));
    XLS_ASSIGN_OR_RETURN(fn_header,
                         CcTranspiler::TranslateHeader(function, metadata,
                                                       header_path.string()));
  } else if (transpiler_type == "interpreted_tfhe") {
    XLS_ASSIGN_OR_RETURN(
        fn_body, InterpretedTfheTranspiler::Translate(function, metadata));
    XLS_ASSIGN_OR_RETURN(fn_header,
                         InterpretedTfheTranspiler::TranslateHeader(
                             function, metadata, header_path.string()));
  } else if (transpiler_type == "tfhe") {
    XLS_ASSIGN_OR_RETURN(fn_body,
                         TfheTranspiler::Translate(function, metadata));
    XLS_ASSIGN_OR_RETURN(fn_header,
                         TfheTranspiler::TranslateHeader(function, metadata,
                                                         header_path.string()));
  } else {
    return absl::InvalidArgumentError(
        absl::StrCat("Invalid transpiler type: ", transpiler_type));
  }

  if (header_path == "-") {
    std::cout << fn_header << std::endl;
  } else {
    XLS_RETURN_IF_ERROR(xls::SetFileContents(header_path, fn_header));
  }
  if (cc_path == "-") {
    std::cout << fn_body << std::endl;
  } else {
    XLS_RETURN_IF_ERROR(xls::SetFileContents(cc_path, fn_body));
  }

  return absl::OkStatus();
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage(argv[0]);
  absl::ParseCommandLine(argc, argv);

  std::string metadata_path = absl::GetFlag(FLAGS_metadata_path);
  if (metadata_path.empty()) {
    std::cerr << "--metadata_path must be specified." << std::endl;
    return 1;
  }

  absl::optional<std::filesystem::path> output_ir_path;
  if (!absl::GetFlag(FLAGS_output_ir_path).empty()) {
    output_ir_path = absl::GetFlag(FLAGS_output_ir_path);
  }

  absl::Status status = fully_homomorphic_encryption::transpiler::RealMain(
      absl::GetFlag(FLAGS_ir_path), output_ir_path,
      absl::GetFlag(FLAGS_header_path), absl::GetFlag(FLAGS_cc_path),
      absl::GetFlag(FLAGS_booleanify_main_path),
      absl::GetFlag(FLAGS_opt_main_path), metadata_path,
      absl::GetFlag(FLAGS_opt_passes));
  if (!status.ok()) {
    std::cerr << status.ToString() << std::endl;
    return 1;
  }

  return 0;
}
