#
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
load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

fhe_cc_library(
    name = "string_reverse_tfhe",
    src = "string_reverse.cc",
    hdrs = ["string_reverse.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_reverse_interpreted_tfhe",
    src = "string_reverse.cc",
    hdrs = ["string_reverse.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_reverse_openfhe",
    src = "string_reverse.cc",
    hdrs = ["string_reverse.h"],
    encryption = "openfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_reverse_interpreted_openfhe",
    src = "string_reverse.cc",
    hdrs = ["string_reverse.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_reverse_cleartext",
    src = "string_reverse.cc",
    hdrs = ["string_reverse.h"],
    encryption = "cleartext",
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_binary(
    name = "string_reverse_tfhe_testbench",
    srcs = [
        "string_reverse.h",
        "string_reverse_tfhe_testbench.cc",
    ],
    deps = [
        ":string_reverse_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "string_reverse_interpreted_tfhe_testbench",
    srcs = [
        "string_reverse.h",
        "string_reverse_tfhe_testbench.cc",
    ],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":string_reverse_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "string_reverse_openfhe_testbench",
    srcs = [
        "string_reverse.h",
        "string_reverse_openfhe_testbench.cc",
    ],
    deps = [
        ":string_reverse_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "string_reverse_interpreted_openfhe_testbench",
    srcs = [
        "string_reverse.h",
        "string_reverse_openfhe_testbench.cc",
    ],
    copts = ["-DUSE_INTERPRETED_OPENFHE"],
    deps = [
        ":string_reverse_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "string_reverse_cleartext_testbench",
    srcs = [
        "string_reverse.h",
        "string_reverse_cleartext_testbench.cc",
    ],
    deps = [
        ":string_reverse_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_library(
    name = "string_reverse",
    srcs = ["string_reverse.cc"],
    hdrs = ["string_reverse.h"],
    deps = [
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "string_reverse_testbench",
    srcs = ["string_reverse_testbench.cc"],
    deps = [
        ":string_reverse",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@tfhe//:libtfhe",
    ],
)
