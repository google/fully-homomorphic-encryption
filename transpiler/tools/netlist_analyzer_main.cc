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

#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "transpiler/netlist_utils.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/status/status_macros.h"
#include "xls/netlist/cell_library.h"
#include "xls/netlist/function_extractor.h"
#include "xls/netlist/netlist_parser.h"

ABSL_FLAG(std::string, cell_library,
          "privacy/private_computing/fhe/tools/testdata/simple_cell.lib",
          "Cell library to use for interpretation.");
ABSL_FLAG(std::string, netlist,
          "privacy/private_computing/fhe/tools/testdata/ifte.v",
          "Path to the netlist to interpret.");
ABSL_FLAG(std::string, output_path, "",
          "Path to which to write the generated analysis file. "
          "If unspecified, output will be written to stdout.");

namespace xls {
absl::StatusOr<netlist::AbstractCellLibrary<bool>> GetCellLibrary(
    absl::string_view cell_library_path) {
  XLS_ASSIGN_OR_RETURN(std::string cell_library_text,
                       GetFileContents(cell_library_path));
  XLS_ASSIGN_OR_RETURN(
      auto char_stream,
      netlist::cell_lib::CharStream::FromText(cell_library_text));
  XLS_ASSIGN_OR_RETURN(netlist::CellLibraryProto lib_proto,
                       netlist::function::ExtractFunctions(&char_stream));
  return netlist::AbstractCellLibrary<bool>::FromProto(lib_proto);
}

absl::Status RealMain(absl::string_view netlist_path,
                      absl::string_view cell_library_path,
                      absl::string_view output_path = "") {
  XLS_ASSIGN_OR_RETURN(auto cell_library, GetCellLibrary(cell_library_path));

  XLS_ASSIGN_OR_RETURN(std::string netlist_text, GetFileContents(netlist_path));
  netlist::rtl::Scanner scanner(netlist_text);
  XLS_ASSIGN_OR_RETURN(auto netlist,
                       netlist::rtl::AbstractParser<bool>::ParseNetlist(
                           &cell_library, &scanner));
  const xls::netlist::rtl::AbstractModule<bool>* module =
      netlist->modules()[0].get();

  XLS_CHECK(module != nullptr) << "No modules found.";

  XLS_ASSIGN_OR_RETURN(
      auto sorted_cell_names,
      fully_homomorphic_encryption::transpiler::LevelSortedCellNames(*module));

  std::vector<std::string> output_lines;
  output_lines.push_back(
      absl::Substitute("Cell Library: $0", cell_library_path));
  output_lines.push_back(absl::Substitute("Netlist Library: $0", netlist_path));
  output_lines.push_back("\nLevel details:");
  int total_gates = 0;
  int max_level_size = 0;
  for (int i = 0; i < sorted_cell_names.size(); i++) {
    const auto& level = sorted_cell_names[i];
    total_gates += level.size();
    absl::flat_hash_map<absl::string_view, int> cell_counts;
    for (const auto& cell_id : level) {
      max_level_size =
          level.size() > max_level_size ? level.size() : max_level_size;
      XLS_ASSIGN_OR_RETURN(const auto& cell, module->ResolveCell(cell_id));
      if (cell_counts.contains(cell->cell_library_entry()->name())) {
        cell_counts[cell->cell_library_entry()->name()]++;
      } else {
        cell_counts[cell->cell_library_entry()->name()] = 1;
      }
    }
    output_lines.push_back(absl::Substitute(
        "Level $0($1): $2", i, level.size(),
        absl::StrJoin(cell_counts, ", ", absl::PairFormatter(":"))));
  }
  output_lines.push_back(
      absl::Substitute("\nTotal number of gates: $0", total_gates));
  output_lines.push_back(absl::Substitute("Number of levels(height): $0",
                                          sorted_cell_names.size()));
  output_lines.push_back(
      absl::Substitute("Widest level(width): $0\n", max_level_size));
  std::string output = absl::StrJoin(output_lines, "\n");
  if (!output_path.empty())
    return xls::SetFileContents(output_path, output);
  else
    std::cout << output << std::endl;

  return absl::OkStatus();
}

}  // namespace xls

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage(argv[0]);
  absl::ParseCommandLine(argc, argv);

  std::string cell_library_path = absl::GetFlag(FLAGS_cell_library);
  XLS_CHECK(!cell_library_path.empty()) << " --cell_library must be specified.";
  std::cout << "Using Cell Library: " << cell_library_path << std::endl;

  std::string netlist_path = absl::GetFlag(FLAGS_netlist);
  XLS_CHECK(!netlist_path.empty()) << "--netlist must be specified.";
  std::cout << "Using Netlist: " << netlist_path << std::endl;

  XLS_CHECK_OK(xls::RealMain(netlist_path, cell_library_path,
                             absl::GetFlag(FLAGS_output_path)));

  return 0;
}
