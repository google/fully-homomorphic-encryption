load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

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

licenses(["notice"])

fhe_cc_library(
    name = "array_of_structs_cleartext",
    src = "array_of_structs.cc",
    hdrs = ["array_of_structs.h"],
    encryption = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "array_of_structs_yosys_interpreted_cleartext",
    src = "array_of_structs.cc",
    hdrs = ["array_of_structs.h"],
    encryption = "cleartext",
    interpreter = True,
    optimizer = "yosys",
)

cc_binary(
    name = "array_of_structs_cleartext_testbench",
    srcs = ["array_of_structs_cleartext_testbench.cc"],
    deps = [
        ":array_of_structs_cleartext",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "array_of_structs_yosys_interpreted_cleartext_testbench",
    srcs = ["array_of_structs_cleartext_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_YOSYS"],
    deps = [
        ":array_of_structs_yosys_interpreted_cleartext",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

fhe_cc_library(
    name = "array_of_structs_tfhe",
    src = "array_of_structs.cc",
    hdrs = ["array_of_structs.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "array_of_structs_yosys_interpreted_tfhe",
    src = "array_of_structs.cc",
    hdrs = ["array_of_structs.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "yosys",
)

cc_binary(
    name = "array_of_structs_tfhe_testbench",
    srcs = ["array_of_structs_tfhe_testbench.cc"],
    deps = [
        ":array_of_structs_tfhe",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "array_of_structs_yosys_interpreted_tfhe_testbench",
    srcs = ["array_of_structs_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_YOSYS"],
    deps = [
        ":array_of_structs_yosys_interpreted_tfhe",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

fhe_cc_library(
    name = "array_of_structs_3d_cleartext",
    src = "array_of_structs_3d.cc",
    hdrs = ["array_of_structs.h"],
    encryption = "cleartext",
)

fhe_cc_library(
    name = "array_of_structs_3d_yosys_interpreted_cleartext",
    src = "array_of_structs_3d.cc",
    hdrs = ["array_of_structs.h"],
    encryption = "cleartext",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "array_of_structs_3d_interpreted_tfhe",
    src = "array_of_structs_3d.cc",
    hdrs = ["array_of_structs.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "xls",
)

fhe_cc_library(
    name = "array_of_structs_3d_yosys_interpreted_tfhe",
    src = "array_of_structs_3d.cc",
    hdrs = ["array_of_structs.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "array_of_structs_3d_interpreted_openfhe",
    src = "array_of_structs_3d.cc",
    hdrs = ["array_of_structs.h"],
    encryption = "openfhe",
    interpreter = True,
    optimizer = "xls",
)

fhe_cc_library(
    name = "array_of_structs_3d_yosys_interpreted_openfhe",
    src = "array_of_structs_3d.cc",
    hdrs = ["array_of_structs.h"],
    encryption = "openfhe",
    interpreter = True,
    optimizer = "yosys",
)

cc_binary(
    name = "array_of_structs_3d_cleartext_testbench",
    srcs = ["array_of_structs_3d_cleartext_testbench.cc"],
    deps = [
        ":array_of_structs_3d_cleartext",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "array_of_structs_3d_yosys_interpreted_cleartext_testbench",
    srcs = ["array_of_structs_3d_cleartext_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_YOSYS"],
    deps = [
        ":array_of_structs_3d_yosys_interpreted_cleartext",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "array_of_structs_3d_interpreted_tfhe_testbench",
    srcs = ["array_of_structs_3d_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":array_of_structs_3d_interpreted_tfhe",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "array_of_structs_3d_yosys_interpreted_openfhe_testbench",
    srcs = ["array_of_structs_3d_openfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_OPENFHE"],
    deps = [
        ":array_of_structs_3d_yosys_interpreted_openfhe",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "array_of_structs_3d_interpreted_openfhe_testbench",
    srcs = ["array_of_structs_3d_openfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_OPENFHE"],
    deps = [
        ":array_of_structs_3d_interpreted_openfhe",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_library(
    name = "array_of_array_of_structs",
    srcs = ["array_of_array_of_structs.cc"],
    hdrs = ["array_of_array_of_structs.h"],
)

fhe_cc_library(
    name = "array_of_array_of_structs_tfhe",
    src = "array_of_array_of_structs.cc",
    hdrs = ["array_of_array_of_structs.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "array_of_array_of_structs_openfhe",
    src = "array_of_array_of_structs.cc",
    hdrs = ["array_of_array_of_structs.h"],
    encryption = "openfhe",
    optimizer = "xls",
)

cc_binary(
    name = "array_of_array_of_structs_tfhe_testbench",
    srcs = ["array_of_array_of_structs_tfhe_testbench.cc"],
    deps = [
        ":array_of_array_of_structs_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "array_of_array_of_structs_openfhe_testbench",
    srcs = ["array_of_array_of_structs_openfhe_testbench.cc"],
    deps = [
        ":array_of_array_of_structs_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_library(
    name = "return_struct",
    srcs = ["return_struct.cc"],
    hdrs = ["return_struct.h"],
)

fhe_cc_library(
    name = "return_struct_cleartext",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    encryption = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "return_struct_yosys_cleartext",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    encryption = "cleartext",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "return_struct_tfhe",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "return_struct_yosys_interpreted_tfhe",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "return_struct_interpreted_tfhe",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "xls",
)

fhe_cc_library(
    name = "return_struct_openfhe",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    encryption = "openfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "return_struct_interpreted_openfhe",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    encryption = "openfhe",
    interpreter = True,
    optimizer = "xls",
)

cc_binary(
    name = "return_struct_tfhe_testbench",
    srcs = ["return_struct_tfhe_testbench.cc"],
    deps = [
        ":return_struct_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "return_struct_interpreted_tfhe_testbench",
    srcs = ["return_struct_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":return_struct_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "return_struct_yosys_interpreted_tfhe_testbench",
    srcs = ["return_struct_tfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":return_struct_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "return_struct_openfhe_testbench",
    srcs = ["return_struct_openfhe_testbench.cc"],
    deps = [
        ":return_struct_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "return_struct_interpreted_openfhe_testbench",
    srcs = ["return_struct_openfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_OPENFHE"],
    deps = [
        ":return_struct_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "return_struct_cleartext_testbench",
    srcs = ["return_struct_cleartext_testbench.cc"],
    deps = [
        ":return_struct_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "return_struct_yosys_cleartext_testbench",
    srcs = ["return_struct_cleartext_testbench.cc"],
    copts = ["-DUSE_YOSYS_CLEARTEXT"],
    deps = [
        ":return_struct_yosys_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_library(
    name = "return_struct_with_inout",
    srcs = ["return_struct_with_inout.cc"],
    hdrs = ["return_struct_with_inout.h"],
)

fhe_cc_library(
    name = "return_struct_with_inout_tfhe",
    src = "return_struct_with_inout.cc",
    hdrs = ["return_struct_with_inout.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "return_struct_with_inout_interpreted_tfhe",
    src = "return_struct_with_inout.cc",
    hdrs = ["return_struct_with_inout.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "xls",
)

fhe_cc_library(
    name = "return_struct_with_inout_openfhe",
    src = "return_struct_with_inout.cc",
    hdrs = ["return_struct_with_inout.h"],
    encryption = "openfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "return_struct_with_inout_yosys_interpreted_tfhe",
    src = "return_struct_with_inout.cc",
    hdrs = ["return_struct_with_inout.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "yosys",
)

cc_binary(
    name = "return_struct_with_inout_tfhe_testbench",
    srcs = ["return_struct_with_inout_tfhe_testbench.cc"],
    deps = [
        ":return_struct_with_inout_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "return_struct_with_inout_interpreted_tfhe_testbench",
    srcs = ["return_struct_with_inout_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":return_struct_with_inout_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "return_struct_with_inout_yosys_interpreted_tfhe_testbench",
    srcs = ["return_struct_with_inout_tfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":return_struct_with_inout_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "return_struct_with_inout_openfhe_testbench",
    srcs = ["return_struct_with_inout_openfhe_testbench.cc"],
    deps = [
        ":return_struct_with_inout_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_library(
    name = "simple_struct",
    srcs = ["simple_struct.cc"],
    hdrs = ["simple_struct.h"],
)

fhe_cc_library(
    name = "simple_struct_tfhe",
    src = "simple_struct.cc",
    hdrs = ["simple_struct.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "simple_struct_openfhe",
    src = "simple_struct.cc",
    hdrs = ["simple_struct.h"],
    encryption = "openfhe",
    optimizer = "xls",
)

cc_binary(
    name = "simple_struct_tfhe_testbench",
    srcs = ["simple_struct_tfhe_testbench.cc"],
    deps = [
        ":simple_struct_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "simple_struct_openfhe_testbench",
    srcs = ["simple_struct_openfhe_testbench.cc"],
    deps = [
        ":simple_struct_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_library(
    name = "struct_of_structs",
    srcs = ["struct_of_structs.cc"],
    hdrs = ["struct_of_structs.h"],
)

fhe_cc_library(
    name = "struct_of_structs_tfhe",
    src = "struct_of_structs.cc",
    hdrs = ["struct_of_structs.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "struct_of_structs_cleartext",
    src = "struct_of_structs.cc",
    hdrs = ["struct_of_structs.h"],
    encryption = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "struct_of_structs_yosys_cleartext",
    src = "struct_of_structs.cc",
    hdrs = ["struct_of_structs.h"],
    encryption = "cleartext",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "struct_of_structs_yosys_interpreted_tfhe",
    src = "struct_of_structs.cc",
    hdrs = ["struct_of_structs.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "yosys",
)

cc_binary(
    name = "struct_of_structs_cleartext_testbench",
    srcs = ["struct_of_structs_cleartext_testbench.cc"],
    deps = [
        ":struct_of_structs_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "struct_of_structs_yosys_cleartext_testbench",
    srcs = ["struct_of_structs_cleartext_testbench.cc"],
    copts = ["-DUSE_YOSYS_CLEARTEXT"],
    deps = [
        ":struct_of_structs_yosys_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "struct_of_structs_tfhe_testbench",
    srcs = ["struct_of_structs_tfhe_testbench.cc"],
    deps = [
        ":struct_of_structs_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "struct_of_structs_yosys_interpreted_tfhe_testbench",
    srcs = ["struct_of_structs_tfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":struct_of_structs_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_library(
    name = "struct_with_array",
    srcs = ["struct_with_array.cc"],
    hdrs = ["struct_with_array.h"],
)

fhe_cc_library(
    name = "struct_with_array_tfhe",
    src = "struct_with_array.cc",
    hdrs = ["struct_with_array.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "struct_with_array_interpreted_tfhe",
    src = "struct_with_array.cc",
    hdrs = ["struct_with_array.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "xls",
)

fhe_cc_library(
    name = "struct_with_array_cleartext",
    src = "struct_with_array.cc",
    hdrs = ["struct_with_array.h"],
    encryption = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "struct_with_array_yosys_cleartext",
    src = "struct_with_array.cc",
    hdrs = ["struct_with_array.h"],
    encryption = "cleartext",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "struct_with_array_yosys_interpreted_tfhe",
    src = "struct_with_array.cc",
    hdrs = ["struct_with_array.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "yosys",
)

cc_binary(
    name = "struct_with_array_tfhe_testbench",
    srcs = ["struct_with_array_tfhe_testbench.cc"],
    deps = [
        ":struct_with_array_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "struct_with_array_interpreted_tfhe_testbench",
    srcs = ["struct_with_array_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":struct_with_array_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "struct_with_array_yosys_interpreted_tfhe_testbench",
    srcs = ["struct_with_array_tfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":struct_with_array_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "struct_with_array_cleartext_testbench",
    srcs = ["struct_with_array_cleartext_testbench.cc"],
    deps = [
        ":struct_with_array_cleartext",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "struct_with_array_yosys_cleartext_testbench",
    srcs = ["struct_with_array_cleartext_testbench.cc"],
    copts = ["-DUSE_YOSYS_CLEARTEXT"],
    deps = [
        ":struct_with_array_yosys_cleartext",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_library(
    name = "struct_with_struct_array",
    srcs = ["struct_with_struct_array.cc"],
    hdrs = ["struct_with_struct_array.h"],
)

fhe_cc_library(
    name = "struct_with_struct_array_tfhe",
    src = "struct_with_struct_array.cc",
    hdrs = ["struct_with_struct_array.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "struct_with_struct_array_interpreted_tfhe",
    src = "struct_with_struct_array.cc",
    hdrs = ["struct_with_struct_array.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "xls",
)

fhe_cc_library(
    name = "struct_with_struct_array_yosys_interpreted_tfhe",
    src = "struct_with_struct_array.cc",
    hdrs = ["struct_with_struct_array.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "yosys",
)

cc_binary(
    name = "struct_with_struct_array_tfhe_testbench",
    srcs = ["struct_with_struct_array_tfhe_testbench.cc"],
    deps = [
        ":struct_with_struct_array_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "struct_with_struct_array_interpreted_tfhe_testbench",
    srcs = ["struct_with_struct_array_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":struct_with_struct_array_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "struct_with_struct_array_yosys_interpreted_tfhe_testbench",
    srcs = ["struct_with_struct_array_tfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":struct_with_struct_array_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_library(
    name = "templated_struct",
    srcs = ["templated_struct.cc"],
    hdrs = ["templated_struct.h"],
)

fhe_cc_library(
    name = "templated_struct_cleartext",
    src = "templated_struct.cc",
    hdrs = ["templated_struct.h"],
    encryption = "cleartext",
)

cc_binary(
    name = "templated_struct_cleartext_testbench",
    srcs = ["templated_struct_cleartext_testbench.cc"],
    deps = [
        ":templated_struct",
        ":templated_struct_cleartext",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_library(
    name = "templated_struct2",
    srcs = ["templated_struct2.cc"],
    hdrs = ["templated_struct2.h"],
)

fhe_cc_library(
    name = "templated_struct2_cleartext",
    src = "templated_struct2.cc",
    hdrs = ["templated_struct2.h"],
    encryption = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "templated_struct2_interpreted_tfhe",
    src = "templated_struct2.cc",
    hdrs = ["templated_struct2.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "xls",
)

fhe_cc_library(
    name = "templated_struct2_interpreted_openfhe",
    src = "templated_struct2.cc",
    hdrs = ["templated_struct2.h"],
    encryption = "openfhe",
    interpreter = True,
    optimizer = "xls",
)

fhe_cc_library(
    name = "templated_struct2_yosys_interpreted_cleartext",
    src = "templated_struct2.cc",
    hdrs = ["templated_struct2.h"],
    encryption = "cleartext",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "templated_struct2_yosys_interpreted_tfhe",
    src = "templated_struct2.cc",
    hdrs = ["templated_struct2.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "templated_struct2_yosys_interpreted_openfhe",
    src = "templated_struct2.cc",
    hdrs = ["templated_struct2.h"],
    encryption = "openfhe",
    interpreter = True,
    optimizer = "yosys",
)

cc_test(
    name = "templated_struct2_cleartext_test",
    srcs = ["templated_struct2_cleartext_test.cc"],
    deps = [
        ":templated_struct2",
        ":templated_struct2_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_test(
    name = "templated_struct2_interpreted_tfhe_test",
    srcs = ["templated_struct2_tfhe_test.cc"],
    deps = [
        ":templated_struct2",
        ":templated_struct2_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "templated_struct2_interpreted_openfhe_test",
    srcs = ["templated_struct2_openfhe_test.cc"],
    deps = [
        ":templated_struct2",
        ":templated_struct2_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_test(
    name = "templated_struct2_yosys_interpreted_cleartext_test",
    srcs = ["templated_struct2_cleartext_test.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_CLEARTEXT"],
    deps = [
        ":templated_struct2",
        ":templated_struct2_yosys_interpreted_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_test(
    name = "templated_struct2_yosys_interpreted_tfhe_test",
    srcs = ["templated_struct2_tfhe_test.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":templated_struct2",
        ":templated_struct2_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "templated_struct2_yosys_interpreted_openfhe_test",
    srcs = ["templated_struct2_openfhe_test.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_OPENFHE"],
    deps = [
        ":templated_struct2",
        ":templated_struct2_yosys_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)
