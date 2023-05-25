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

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

# TODO: Consider the project-wide visibility policy.
package(
    default_applicable_licenses = ["@com_google_fully_homomorphic_encryption//:license"],
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

cc_binary(
    name = "struct_transpiler",
    srcs = ["struct_transpiler_main.cc"],
    deps = [
        ":convert_struct_to_encoded",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/file:filesystem",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
    ],
)

cc_library(
    name = "convert_struct_to_encoded",
    srcs = ["convert_struct_to_encoded.cc"],
    hdrs = ["convert_struct_to_encoded.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//transpiler:common_transpiler",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
    ],
)

cc_test(
    name = "convert_struct_to_encoded_test",
    srcs = ["convert_struct_to_encoded_test.cc"],
    deps = [
        ":convert_struct_to_encoded",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_protobuf//:protobuf",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
    ],
)
