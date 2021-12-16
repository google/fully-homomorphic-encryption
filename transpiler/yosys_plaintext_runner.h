#ifndef THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_PLAINTEXT_RUNNER_H_
#define THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_PLAINTEXT_RUNNER_H_

#include <memory>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/netlist/cell_library.h"
#include "xls/netlist/function_extractor.h"
#include "xls/netlist/interpreter.h"
#include "xls/netlist/lib_parser.h"
#include "xls/netlist/netlist_parser.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

class YosysRunner {
 public:
  static absl::Status CreateAndRun(const std::string& netlist_text,
                                   const std::string& metadata_text,
                                   const std::string& liberty_text,
                                   absl::Span<bool> result,
                                   std::vector<absl::Span<bool>> args);
};

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // THIRD_PARTY_FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_YOSYS_PLAINTEXT_RUNNER_H_
