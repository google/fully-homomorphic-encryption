#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_RUST_YOSYS_TRANSPILER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_RUST_YOSYS_TRANSPILER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/netlist/cell_library.h"
#include "xls/netlist/function_extractor.h"
#include "xls/netlist/lib_parser.h"
#include "xls/netlist/netlist.h"
#include "xls/netlist/netlist_parser.h"

namespace fhe {
namespace rust {
namespace transpiler {

struct BuildGateOpsOutput {
  std::string task_blocks;
  int level_count;
  absl::flat_hash_set<int> levels_with_prune;
};

class YosysTfheRsTranspiler {
 public:
  // Takes ownership of the input netlist ptr
  YosysTfheRsTranspiler(
      const xlscc_metadata::MetadataOutput& metadata,
      std::unique_ptr<::xls::netlist::rtl::AbstractNetlist<bool>> netlist)
      : metadata_(metadata), netlist_(std::move(netlist)) {}

  ~YosysTfheRsTranspiler() = default;
  // Converts Verilog netlist to a tfhe-rs Rust program, using LUT gate ops
  // from the tfhe::shortint crate. The `parallelism` argument specifies how
  // many devices are available to evaluate gates in parallel, with zero
  // defaulting to unbounded parallelism.
  absl::StatusOr<std::string> Translate(int parallelism);

  // An override of Translate that evaluates gates without limiting the
  // parallelism to a specific value (unbounded parallelism).
  absl::StatusOr<std::string> Translate() { return Translate(0); }

 private:
  const xlscc_metadata::MetadataOutput& metadata_;
  std::unique_ptr<::xls::netlist::rtl::AbstractNetlist<bool>> netlist_;

  const xls::netlist::rtl::AbstractModule<bool>& module() {
    return *netlist_->modules().front();
  }

  absl::StatusOr<BuildGateOpsOutput> BuildGateOps(int parallelism);
  absl::StatusOr<std::string> FunctionSignature();
  absl::StatusOr<std::string> AssignOutputs();
};

}  // namespace transpiler
}  // namespace rust
}  // namespace fhe

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_RUST_YOSYS_TRANSPILER_H_
