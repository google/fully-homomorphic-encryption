#include "transpiler/metadata_utils.h"

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "nlohmann/json.hpp"
#include "transpiler/netlist_utils.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/netlist/netlist.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

namespace {

absl::Status SetType(xlscc_metadata::Type* type, nlohmann::json type_data) {
  auto type_obj = (*type_data.items().begin());
  auto type_key = type_obj.key();
  auto type_value = type_obj.value();
  if (type_key == "integer") {
    // Assume that all HEIR-produced integer types are integral (not chars).
    type->mutable_as_int()->set_width(type_value["width"]);
    type->mutable_as_int()->set_is_signed(type_value["is_signed"]);
    return absl::OkStatus();
  }
  if (type_key == "memref") {
    auto shapeIt = type_value["shape"].begin();
    type->mutable_as_array()->set_size(*shapeIt++);
    auto element_type = type->mutable_as_array()->mutable_element_type();
    while (shapeIt != type_value["shape"].end()) {
      element_type->mutable_as_array()->set_size(*shapeIt++);
      element_type = element_type->mutable_as_array()->mutable_element_type();
    }
    return SetType(element_type, type_value["element_type"]);
  }
  return absl::InvalidArgumentError("unexpected type");
}

absl::StatusOr<nlohmann::json> GetJsonValue(nlohmann::json j,
                                            absl::string_view key) {
  if (!j.contains(key)) {
    return absl::NotFoundError(key);
  }
  return j[key.begin()];
}

}  // namespace

absl::StatusOr<xlscc_metadata::MetadataOutput> CreateMetadataFromHeirJson(
    absl::string_view metadata_str,
    const std::unique_ptr<xls::netlist::rtl::AbstractModule<bool>>& module) {
  auto metadata = nlohmann::json::parse(metadata_str, nullptr, false);
  if (metadata.is_discarded()) {
    return absl::InvalidArgumentError("error parsing metadata JSON");
  }

  XLS_ASSIGN_OR_RETURN(nlohmann::json funcs,
                       GetJsonValue(metadata, "functions"));
  nlohmann::json top_level_func;
  for (const auto& func : funcs) {
    XLS_ASSIGN_OR_RETURN(std::string func_name, GetJsonValue(func, "name"));
    // Currently, we assume all HEIR-output metadata use main as the top-level
    // func.
    if (func_name == "main") {
      top_level_func = func;
      break;
    }
  }
  if (top_level_func.empty()) {
    return absl::InvalidArgumentError("expected main function in metadata");
  }

  // Gather an ordered list of the input arguments from the netlist.
  std::vector<std::string> input_stems;
  for (const auto& input : module->inputs()) {
    auto stem = NetRefStem(input->name());
    if (std::find(input_stems.begin(), input_stems.end(), stem) ==
        input_stems.end()) {
      input_stems.push_back(stem);
    }
  }

  xlscc_metadata::FunctionPrototype top_func_proto;
  top_func_proto.mutable_name()->set_name(module->name());

  for (const auto& param : top_level_func["params"]) {
    xlscc_metadata::FunctionParameter* param_proto =
        top_func_proto.add_params();
    // Set the type name based on its index in the netlist.
    XLS_ASSIGN_OR_RETURN(int param_index, GetJsonValue(param, "index"));
    param_proto->set_name(input_stems[param_index]);
    // Set parameter type.
    XLS_ASSIGN_OR_RETURN(nlohmann::json param_type,
                         GetJsonValue(param, "type"));
    XLS_RETURN_IF_ERROR(SetType(param_proto->mutable_type(), param_type));
  }

  XLS_ASSIGN_OR_RETURN(nlohmann::json return_type,
                       GetJsonValue(top_level_func, "return_type"));
  XLS_RETURN_IF_ERROR(
      SetType(top_func_proto.mutable_return_type(), return_type));

  xlscc_metadata::MetadataOutput output;
  output.mutable_top_func_proto()->Swap(&top_func_proto);
  return output;
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
