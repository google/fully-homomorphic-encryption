// Takes a verilog netlist ir file output by Yosys, and produces an FHE .rs file
// that targets the tfhe-rs cryptosystem backend.

#include <iostream>
#include <ostream>
#include <string>
#include <utility>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "transpiler/netlist_utils.h"
#include "transpiler/rust/yosys_transpiler.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"

ABSL_FLAG(std::string, ir_path, "", "Path to the input IR to process.");
ABSL_FLAG(std::string, metadata_path, "",
          "Path to a [binary-format] xlscc MetadataOutput protobuf "
          "containing data about the function to transpile.");
ABSL_FLAG(std::string, rs_out, "",
          "Path to a .rs output file for the resulting generated code. If "
          "empty, the generated code is dumped to STDOUT.");
ABSL_FLAG(std::string, liberty_path, "",
          "Path to cell-definition library in Liberty format; required when "
          "using the Yosys optimizer, and otherwise has no effect.");
ABSL_FLAG(int, parallelism, 0,
          "The degree of parallelism to use for scheduling circuit execution. "
          "If zero, the circuit is scheduled without a bound on parallelism.");

namespace fhe {
namespace rust {
namespace transpiler {

absl::Status RealMain() {
  const std::string ir_path = absl::GetFlag(FLAGS_ir_path);
  const std::string rs_out = absl::GetFlag(FLAGS_rs_out);
  std::string liberty_path = absl::GetFlag(FLAGS_liberty_path);
  if (liberty_path.empty()) {
    return absl::InvalidArgumentError("--liberty_path required.");
  }

  const std::string metadata_path = absl::GetFlag(FLAGS_metadata_path);
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

  XLS_ASSIGN_OR_RETURN(std::string ir_text, xls::GetFileContents(ir_path));

  int parallelism = absl::GetFlag(FLAGS_parallelism);
  if (parallelism < 0) {
    return absl::InvalidArgumentError(
        "--parallelism must be non-negative, but was: " +
        std::to_string(parallelism));
  }

  XLS_ASSIGN_OR_RETURN(std::string cell_library_text,
                       xls::GetFileContents(liberty_path));

  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<::xls::netlist::rtl::AbstractNetlist<bool>> netlist,
      fully_homomorphic_encryption::transpiler::ParseNetlist(cell_library_text,
                                                             ir_text));

  YosysTfheRsTranspiler transpiler(metadata, std::move(netlist));
  XLS_ASSIGN_OR_RETURN(std::string module_impl,
                       transpiler.Translate(parallelism));

  if (rs_out.empty()) {
    std::cout << module_impl << std::endl;
  } else {
    XLS_RETURN_IF_ERROR(xls::SetFileContents(rs_out, module_impl));
  }

  return absl::OkStatus();
}
}  // namespace transpiler
}  // namespace rust
}  // namespace fhe

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage(argv[0]);
  absl::ParseCommandLine(argc, argv);

  absl::Status status = fhe::rust::transpiler::RealMain();
  if (!status.ok()) {
    std::cerr << status.ToString() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
