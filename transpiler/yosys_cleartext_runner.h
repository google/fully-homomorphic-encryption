#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_CLEARTEXT_RUNNER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_CLEARTEXT_RUNNER_H_

#include <memory>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "google/protobuf/text_format.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/netlist/cell_library.h"
#include "xls/netlist/function_extractor.h"
#include "xls/netlist/interpreter.h"
#include "xls/netlist/lib_parser.h"
#include "xls/netlist/netlist_parser.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

using OpaqueValue = bool;
using BoolValue = bool;

class YosysRunner {
 public:
  // char_stream: value
  //   --> lib_proto: value
  //      --> cell_library: value
  //
  //  netlist_text
  //    --> scanner: value
  //
  //  &cell_library, &scanner
  //    --> netlist: pointer
  //
  YosysRunner(const std::string& liberty_text, const std::string& netlist_text,
              const std::string& metadata_text)
      : liberty_text_(liberty_text),
        netlist_text_(netlist_text),
        metadata_text_(metadata_text),
        state_(nullptr) {}

  absl::Status InitializeOnce(
      const xls::netlist::rtl::CellToOutputEvalFns<BoolValue>& eval_fns);

  absl::Status Run(absl::Span<OpaqueValue> result,
                   std::vector<absl::Span<const OpaqueValue>> in_args,
                   std::vector<absl::Span<OpaqueValue>> inout_args);

  std::unique_ptr<BoolValue> CreateBoolValue(OpaqueValue in) {
    return std::make_unique<BoolValue>(in);
  }

 private:
  struct YosysRunnerState {
    YosysRunnerState(xls::netlist::cell_lib::CharStream char_stream,
                     xls::netlist::rtl::Scanner scanner)
        : zero_{false},
          one_{true},
          char_stream_(std::move(char_stream)),
          lib_proto_(*xls::netlist::function::ExtractFunctions(&char_stream_)),
          cell_library_(
              *xls::netlist::AbstractCellLibrary<BoolValue>::FromProto(
                  lib_proto_, zero_, one_)),
          scanner_(scanner) {}

    absl::Status Run(absl::Span<OpaqueValue> result,
                     std::vector<absl::Span<const OpaqueValue>> in_args,
                     std::vector<absl::Span<OpaqueValue>> inout_args);

    BoolValue zero_;
    BoolValue one_;
    xls::netlist::cell_lib::CharStream char_stream_;
    xls::netlist::CellLibraryProto lib_proto_;
    xls::netlist::AbstractCellLibrary<BoolValue> cell_library_;
    xls::netlist::rtl::Scanner scanner_;
    std::unique_ptr<xls::netlist::rtl::AbstractNetlist<BoolValue>> netlist_;
    xlscc_metadata::MetadataOutput metadata_;
  };

  // The method names are prefixed by "TfheOp_" since they map to the cell
  // library defined specifically for TFHE.  This is despire the fact that we do
  // not perform any actual FHE here.
  absl::StatusOr<BoolValue> TfheOp_inv(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_buffer(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_and2(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_nand2(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_or2(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_andyn2(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_andny2(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_oryn2(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_orny2(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_nor2(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_xor2(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_xnor2(const std::vector<BoolValue>& args);
  absl::StatusOr<BoolValue> TfheOp_imux2(const std::vector<BoolValue>& args);

  const std::string liberty_text_;
  const std::string netlist_text_;
  const std::string metadata_text_;
  std::unique_ptr<YosysRunnerState> state_;
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_CLEARTEXT_RUNNER_H_
