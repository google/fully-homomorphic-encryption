# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#  BUILD rules for tools useful for transpilation and analysis of circuits.

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(
    default_applicable_licenses = ["@com_google_fully_homomorphic_encryption//:license"],
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

cc_binary(
    name = "netlist_analyzer_main",
    srcs = ["netlist_analyzer_main.cc"],
    deps = [
        "//transpiler:netlist_utils",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/file:filesystem",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/netlist:cell_library",
        "@com_google_xls//xls/netlist:function_extractor",
        "@com_google_xls//xls/netlist:netlist_parser",
    ],
)
