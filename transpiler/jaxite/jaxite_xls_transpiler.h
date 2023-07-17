#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_JAXITE_JAXITE_XLS_TRANSPILER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_JAXITE_JAXITE_XLS_TRANSPILER_H_

#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "transpiler/abstract_xls_transpiler.h"
#include "xls/public/ir.h"

namespace fhe {
namespace jaxite {
namespace transpiler {

using ::fully_homomorphic_encryption::transpiler::AbstractXLSTranspiler;

// Converts booleanified XLS functions into Jaxite .py programs, using the
// boolean gate ops from the jaxite_bool package.
class JaxiteXlsTranspiler : public AbstractXLSTranspiler<JaxiteXlsTranspiler> {
 public:
  static absl::StatusOr<std::string> TranslateHeader(
      const xls::Function* function,
      const xlscc_metadata::MetadataOutput& metadata,
      absl::string_view header_path);

  static absl::StatusOr<std::string> FunctionSignature(
      const xls::Function* function,
      const xlscc_metadata::MetadataOutput& metadata);

  static std::string CopyNodeToOutput(absl::string_view output_arg, int offset,
                                      const xls::Node* node);
  static std::string CopyParamToNode(const xls::Node* node,
                                     const xls::Node* param, int offset);
  static std::string InitializeNode(const xls::Node* node);

  static absl::StatusOr<std::string> Execute(const xls::Node* node);

  static absl::StatusOr<std::string> Prelude(
      const xls::Function* function,
      const xlscc_metadata::MetadataOutput& metadata);

  static absl::StatusOr<std::string> Conclusion();
};
}  // namespace transpiler
}  // namespace jaxite
}  // namespace fhe

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_JAXITE_JAXITE_XLS_TRANSPILER_H_
