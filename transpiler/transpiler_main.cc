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

// Takes an booleanified xls ir file and produces an FHE C++ file that uses an
// FHE api.
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
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "transpiler/cleartext_transpiler.h"
#include "transpiler/interpreted_openfhe_transpiler.h"
#include "transpiler/interpreted_tfhe_transpiler.h"
#include "transpiler/openfhe_transpiler.h"
#include "transpiler/pipeline_enums.h"
#include "transpiler/tfhe_transpiler.h"
#include "transpiler/util/subprocess.h"
#include "transpiler/util/temp_file.h"
#include "transpiler/yosys_transpiler.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/public/ir.h"
#include "xls/public/ir_parser.h"
#include "xls/public/value.h"

namespace {

using fully_homomorphic_encryption::transpiler::Encryption;
using fully_homomorphic_encryption::transpiler::Optimizer;

}  // namespace

ABSL_FLAG(std::string, ir_path, "",
          "Path to the booleanified XLS IR to process.");
ABSL_FLAG(std::string, metadata_path, "",
          "Path to a [binary-format] xlscc MetadataOutput protobuf "
          "containing data about the function to transpile.");
ABSL_FLAG(std::string, header_path, "-",
          "Path to generate the C++ header file. If unspecified, output to "
          "stdout");
ABSL_FLAG(std::string, cc_path, "-",
          "Path to generate the C++ source file. If unspecified, output to "
          "stdout after the header file.");
ABSL_FLAG(Optimizer, optimizer, Optimizer::kXLS,
          "Optimizing/lowering pipeline to use in transpilation. Choices are "
          "{xls, yosys}. 'xls' uses the built-in XLS tools to transform the "
          "program into an optimized Boolean circuit; 'yosys' uses Yosys to "
          "synthesize a circuit that targets the given backend.");
ABSL_FLAG(std::string, liberty_path, "",
          "Path to cell-definition library in Liberty format; required when "
          "using the Yosys optimizer, and otherwise has no effect.");
ABSL_FLAG(Encryption, encryption, Encryption::kTFHE,
          "FHE encryption scheme used by the resulting program. Choices are "
          "{tfhe, openfhe, cleartext}. 'cleartext' means the program runs in "
          "cleartext, skipping encryption; this has zero security, but is "
          "useful for debugging.");
ABSL_FLAG(std::string, encryption_specific_transpiled_structs_header_path, "",
          "Path to generate the C++ header file. May be omitted, if the source "
          "C++ code does not define any user-data types.");
ABSL_FLAG(bool, interpreter, false,
          "Build a program that invokes a multi-threaded interpreter. If not "
          "set, it instead directly implements the circuit in single-threaded "
          "C++.");
ABSL_FLAG(
    bool, skip_scheme_data_deps, false,
    "When set to True, it causes the transpiler to not emit depednencies "
    "for tfhe_data.h, openfhe_data.h, and cleartext_data.h.  This is used "
    "to avoid circular dependencies when generating C++ libraries for "
    "the numeric primitives.");
ABSL_FLAG(std::vector<std::string>, unwrap, std::vector<std::string>({}),
          "Comma-separated list of struct names to unwrap.");

namespace fully_homomorphic_encryption {
namespace transpiler {

absl::Status RealMain(const std::filesystem::path& ir_path,
                      const std::filesystem::path& header_path,
                      const std::filesystem::path& cc_path,
                      const std::filesystem::path& metadata_path,
                      const std::filesystem::path&
                          encryption_specific_transpiled_structs_header_path,
                      bool skip_scheme_data_deps,
                      const std::vector<std::string>& unwrap) {
  XLS_ASSIGN_OR_RETURN(std::string proto_text,
                       xls::GetFileContents(metadata_path));
  xlscc_metadata::MetadataOutput metadata;
  if (!metadata.ParseFromString(proto_text)) {
    return absl::InvalidArgumentError(
        "Could not parse function metadata proto.");
  }

  Optimizer optimizer = absl::GetFlag(FLAGS_optimizer);
  std::string liberty_path = absl::GetFlag(FLAGS_liberty_path);
  if (optimizer == Optimizer::kYosys && liberty_path.empty()) {
    return absl::InvalidArgumentError(
        "--optimizer=yosys requires --liberty_path.");
  }

  Encryption encryption = absl::GetFlag(FLAGS_encryption);
  bool interpreter = absl::GetFlag(FLAGS_interpreter);

  const std::string& function_name = metadata.top_func_proto().name().name();

  XLS_ASSIGN_OR_RETURN(std::string ir_text, xls::GetFileContents(ir_path));
  std::string fn_body, fn_header;
  if (optimizer == Optimizer::kYosys) {
    if (!interpreter) {
      return absl::UnimplementedError(
          "The Yosys pipeline only implements interpreter execution.");
    }
    XLS_ASSIGN_OR_RETURN(
        fn_header,
        YosysTranspiler::TranslateHeader(
            metadata, header_path.string(), encryption,
            encryption_specific_transpiled_structs_header_path.string(),
            unwrap));
    XLS_ASSIGN_OR_RETURN(std::string cell_library_text,
                         xls::GetFileContents(liberty_path));
    XLS_ASSIGN_OR_RETURN(fn_body,
                         YosysTranspiler::Translate(metadata, cell_library_text,
                                                    ir_text, encryption));
  } else {
    XLS_ASSIGN_OR_RETURN(auto package,
                         xls::ParsePackage(ir_text, /*filename=*/std::nullopt));
    XLS_ASSIGN_OR_RETURN(xls::Function * function,
                         package->GetFunction(function_name));

    switch (encryption) {
      case Encryption::kTFHE: {
        if (interpreter) {
          XLS_ASSIGN_OR_RETURN(fn_body, InterpretedTfheTranspiler::Translate(
                                            function, metadata));
          XLS_ASSIGN_OR_RETURN(
              fn_header,
              InterpretedTfheTranspiler::TranslateHeader(
                  function, metadata, header_path.string(),
                  encryption_specific_transpiled_structs_header_path.string(),
                  skip_scheme_data_deps, unwrap));
        } else {
          XLS_ASSIGN_OR_RETURN(fn_body,
                               TfheTranspiler::Translate(function, metadata));
          XLS_ASSIGN_OR_RETURN(
              fn_header,
              TfheTranspiler::TranslateHeader(
                  function, metadata, header_path.string(),
                  encryption_specific_transpiled_structs_header_path.string(),
                  skip_scheme_data_deps, unwrap));
        }
        break;
      }
      case Encryption::kOpenFHE: {
        if (interpreter) {
          XLS_ASSIGN_OR_RETURN(fn_body, InterpretedOpenFheTranspiler::Translate(
                                            function, metadata));
          XLS_ASSIGN_OR_RETURN(
              fn_header,
              InterpretedOpenFheTranspiler::TranslateHeader(
                  function, metadata, header_path.string(),
                  encryption_specific_transpiled_structs_header_path.string(),
                  skip_scheme_data_deps, unwrap));
        } else {
          XLS_ASSIGN_OR_RETURN(
              fn_body, OpenFheTranspiler::Translate(function, metadata));
          XLS_ASSIGN_OR_RETURN(
              fn_header,
              OpenFheTranspiler::TranslateHeader(
                  function, metadata, header_path.string(),
                  encryption_specific_transpiled_structs_header_path.string(),
                  skip_scheme_data_deps, unwrap));
        }
        break;
      }
      case Encryption::kCleartext: {
        if (interpreter) {
          return absl::UnimplementedError(
              "No XLS interpreter for cleartext is currently implemented.");
        } else {
          XLS_ASSIGN_OR_RETURN(
              fn_body, CleartextTranspiler::Translate(function, metadata));
          XLS_ASSIGN_OR_RETURN(
              fn_header,
              CleartextTranspiler::TranslateHeader(
                  function, metadata, header_path.string(),
                  encryption_specific_transpiled_structs_header_path.string(),
                  skip_scheme_data_deps, unwrap));
        }
        break;
      }
    }
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

  absl::Status status = fully_homomorphic_encryption::transpiler::RealMain(
      absl::GetFlag(FLAGS_ir_path), absl::GetFlag(FLAGS_header_path),
      absl::GetFlag(FLAGS_cc_path), metadata_path,
      absl::GetFlag(FLAGS_encryption_specific_transpiled_structs_header_path),
      absl::GetFlag(FLAGS_skip_scheme_data_deps), absl::GetFlag(FLAGS_unwrap));
  if (!status.ok()) {
    std::cerr << status.ToString() << std::endl;
    return 1;
  }

  return 0;
}
