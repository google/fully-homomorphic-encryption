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

load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

fhe_cc_library(
    name = "string_reverse_tfhe",
    src = "string_reverse.cc",
    hdrs = ["string_reverse.h"],
    num_opt_passes = 2,
)

fhe_cc_library(
    name = "string_reverse_interpreted_tfhe",
    src = "string_reverse.cc",
    hdrs = ["string_reverse.h"],
    backend = "interpreted_tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_reverse_palisade",
    src = "string_reverse.cc",
    hdrs = ["string_reverse.h"],
    backend = "palisade",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_reverse_interpreted_palisade",
    src = "string_reverse.cc",
    hdrs = ["string_reverse.h"],
    backend = "interpreted_palisade",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_reverse_bool",
    src = "string_reverse.cc",
    hdrs = ["string_reverse.h"],
    backend = "cleartext",
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
    name = "string_reverse_palisade_testbench",
    srcs = [
        "string_reverse.h",
        "string_reverse_palisade_testbench.cc",
    ],
    deps = [
        ":string_reverse_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "string_reverse_interpreted_palisade_testbench",
    srcs = [
        "string_reverse.h",
        "string_reverse_palisade_testbench.cc",
    ],
    copts = ["-DUSE_INTERPRETED_PALISADE"],
    deps = [
        ":string_reverse_interpreted_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "string_reverse_bool_testbench",
    srcs = [
        "string_reverse.h",
        "string_reverse_bool_testbench.cc",
    ],
    deps = [
        ":string_reverse_bool",
        "//transpiler/data:boolean_data",
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
