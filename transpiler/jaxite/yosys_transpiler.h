#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_JAXITE_YOSYS_TRANSPILER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_JAXITE_YOSYS_TRANSPILER_H_

#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "xls/netlist/netlist.h"

namespace fhe {
namespace jaxite {
namespace transpiler {

class YosysTranspiler {
 public:
  // Converts Verilog netlist to a Jaxite Python program, using the boolean
  // gate ops from the jaxite_bool package. The `parallelism` argument
  // specifies how many devices are available to evaluate gates in parallel,
  // using pmap.
  static absl::StatusOr<std::string> Translate(
      absl::string_view cell_library_text, absl::string_view netlist_text,
      int parallelism);

  // A version of Translate that evaluates gates serially.
  static absl::StatusOr<std::string> Translate(
      absl::string_view cell_library_text, absl::string_view netlist_text) {
    return Translate(cell_library_text, netlist_text, 0);
  }

 private:
  static absl::StatusOr<std::string> FunctionSignature(
      const xls::netlist::rtl::AbstractModule<bool>& module);

  static absl::StatusOr<std::string> FunctionSetup(
      const xls::netlist::rtl::AbstractModule<bool>& module);

  static absl::StatusOr<std::string> FunctionReturn(
      const xls::netlist::rtl::AbstractModule<bool>& module);

  static absl::StatusOr<std::string> AddGateOps(
      const xls::netlist::rtl::AbstractModule<bool>& module);

  static absl::StatusOr<std::string> AddParallelGateOps(
      const xls::netlist::rtl::AbstractModule<bool>& module,
      int gate_parallelism);

  static absl::StatusOr<std::string> AssignOutputs(
      const xls::netlist::rtl::AbstractModule<bool>& module);
};

}  // namespace transpiler
}  // namespace jaxite
}  // namespace fhe

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_JAXITE_YOSYS_TRANSPILER_H_
