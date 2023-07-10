// Takes a booleanified xls ir file and produces an FHE .py file that targets a
// supported cryptographic backend.

#include <iostream>
#include <ostream>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "transpiler/jaxite/jaxite_xls_transpiler.h"
#include "transpiler/jaxite/yosys_transpiler.h"
#include "transpiler/pipeline_enums.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/public/ir.h"
#include "xls/public/ir_parser.h"

using fully_homomorphic_encryption::transpiler::Optimizer;

ABSL_FLAG(std::string, ir_path, "",
          "Path to the booleanified XLS IR to process.");
ABSL_FLAG(std::string, metadata_path, "",
          "Path to a [binary-format] xlscc MetadataOutput protobuf "
          "containing data about the function to transpile.");
ABSL_FLAG(std::string, py_out, "",
          "Path to a .py output module for the resulting generated code. If "
          "empty, the generated code is dumped to STDOUT.");
ABSL_FLAG(Optimizer, optimizer, Optimizer::kXLS,
          "Optimizing/lowering pipeline to use in transpilation. Choices are "
          "{xls, yosys}. 'xls' uses the built-in XLS tools to transform the "
          "program into an optimized Boolean circuit; 'yosys' uses Yosys to "
          "synthesize a circuit that targets the given backend.");
ABSL_FLAG(std::string, liberty_path, "",
          "Path to cell-definition library in Liberty format; required when "
          "using the Yosys optimizer, and otherwise has no effect.");
ABSL_FLAG(int, parallelism, 0,
          "The degree of parallelism to use for scheduling circuit execution. "
          "If zero, the circuit is scheduled in serial.");

namespace fhe {
namespace jaxite {
namespace transpiler {

// TODO: Add struct transpiler support for Jaxite.
absl::Status RealMain(const std::filesystem::path& ir_path,
                      const std::filesystem::path& py_out) {
  Optimizer optimizer = absl::GetFlag(FLAGS_optimizer);
  std::string liberty_path = absl::GetFlag(FLAGS_liberty_path);
  if (optimizer == Optimizer::kYosys && liberty_path.empty()) {
    return absl::InvalidArgumentError(
        "--optimizer=yosys requires --liberty_path.");
  }

  XLS_ASSIGN_OR_RETURN(std::string ir_text, xls::GetFileContents(ir_path));
  std::string module_impl;
  if (optimizer == Optimizer::kYosys) {
    int parallelism = absl::GetFlag(FLAGS_parallelism);
    if (parallelism < 0) {
      return absl::InvalidArgumentError(
          "--parallelism must be non-negative, but was: " +
          std::to_string(parallelism));
    }

    XLS_ASSIGN_OR_RETURN(std::string cell_library_text,
                         xls::GetFileContents(liberty_path));
    XLS_ASSIGN_OR_RETURN(
        module_impl,
        YosysTranspiler::Translate(cell_library_text, ir_text, parallelism));
  } else {
    std::string metadata_path = absl::GetFlag(FLAGS_metadata_path);
    if (metadata_path.empty()) {
      return absl::InvalidArgumentError(
          "--metadata_path must be specified, but was: " + metadata_path);
    }
    XLS_ASSIGN_OR_RETURN(std::string proto_text,
                         xls::GetFileContents(metadata_path));
    xlscc_metadata::MetadataOutput metadata;
    if (!metadata.ParseFromString(proto_text)) {
      return absl::InvalidArgumentError(
          "Could not parse function metadata proto.");
    }

    // Retrieve XLS function and booleanified content
    const std::string& function_name = metadata.top_func_proto().name().name();
    XLS_ASSIGN_OR_RETURN(auto package,
                         xls::ParsePackage(ir_text, std::string(ir_path)));
    XLS_ASSIGN_OR_RETURN(xls::Function * function,
                         package->GetFunction(function_name));

    // Create .py implementation
    XLS_ASSIGN_OR_RETURN(module_impl,
                         JaxiteXlsTranspiler::Translate(function, metadata));
  }

  if (py_out.empty()) {
    std::cout << module_impl << std::endl;
  } else {
    XLS_RETURN_IF_ERROR(xls::SetFileContents(py_out, module_impl));
  }
  return absl::OkStatus();
}
}  // namespace transpiler
}  // namespace jaxite
}  // namespace fhe

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage(argv[0]);
  absl::ParseCommandLine(argc, argv);

  absl::Status status = fhe::jaxite::transpiler::RealMain(
      absl::GetFlag(FLAGS_ir_path), absl::GetFlag(FLAGS_py_out));
  if (!status.ok()) {
    std::cerr << status.ToString() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
