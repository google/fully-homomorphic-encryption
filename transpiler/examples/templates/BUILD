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

# Build rules for the template-based string reversal example.

load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

fhe_cc_library(
    name = "mul8_tfhe",
    src = "mul8.cc",
    hdrs = [
        "mul.h",
        "mul8.h",
    ],
    num_opt_passes = 2,
)

fhe_cc_library(
    name = "mul16_tfhe",
    src = "mul16.cc",
    hdrs = [
        "mul.h",
        "mul16.h",
    ],
    num_opt_passes = 2,
)

fhe_cc_library(
    name = "mul8_interpreted_tfhe",
    src = "mul8.cc",
    hdrs = [
        "mul.h",
        "mul8.h",
    ],
    num_opt_passes = 2,
    transpiler_type = "interpreted_tfhe",
)

fhe_cc_library(
    name = "mul16_interpreted_tfhe",
    src = "mul16.cc",
    hdrs = [
        "mul.h",
        "mul16.h",
    ],
    num_opt_passes = 2,
    transpiler_type = "interpreted_tfhe",
)

cc_binary(
    name = "mul_tfhe_testbench",
    srcs = ["mul_tfhe_testbench.cc"],
    deps = [
        ":mul16_tfhe",
        ":mul8_tfhe",
        "//transpiler/data:fhe_data",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "mul_interpreted_tfhe_testbench",
    srcs = ["mul_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":mul16_interpreted_tfhe",
        ":mul8_interpreted_tfhe",
        "//transpiler/data:fhe_data",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)
