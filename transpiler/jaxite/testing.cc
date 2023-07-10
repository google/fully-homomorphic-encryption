#include "transpiler/jaxite/testing.h"

#include <vector>

#include "xls/contrib/xlscc/metadata_output.pb.h"

namespace fhe {
namespace jaxite {
namespace transpiler {

xlscc_metadata::MetadataOutput CreateFunctionMetadata(
    const std::vector<Parameter>& params, bool has_return_value) {
  xlscc_metadata::MetadataOutput output;

  for (const Parameter& param : params) {
    xlscc_metadata::FunctionParameter* xls_param =
        output.mutable_top_func_proto()->add_params();
    xls_param->set_name(param.name);
    xls_param->mutable_type()->mutable_as_int()->set_width(kParamBitWidth);
    if (param.in_out) {
      xls_param->set_is_reference(true);
    }
  }

  if (has_return_value) {
    output.mutable_top_func_proto()
        ->mutable_return_type()
        ->mutable_as_int()
        ->set_width(kReturnBitWidth);
  } else {
    output.mutable_top_func_proto()->mutable_return_type()->mutable_as_void();
  }

  return output;
}

}  // namespace transpiler
}  // namespace jaxite
}  // namespace fhe
