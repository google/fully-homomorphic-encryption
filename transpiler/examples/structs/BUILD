load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

cc_library(
    name = "array_of_array_of_structs",
    srcs = ["array_of_array_of_structs.cc"],
    hdrs = ["array_of_array_of_structs.h"],
)

fhe_cc_library(
    name = "array_of_array_of_structs_tfhe",
    src = "array_of_array_of_structs.cc",
    hdrs = ["array_of_array_of_structs.h"],
)

cc_binary(
    name = "array_of_array_of_structs_tfhe_testbench",
    srcs = ["array_of_array_of_structs_tfhe_testbench.cc"],
    deps = [
        ":array_of_array_of_structs_tfhe",
        "//transpiler/data:fhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_library(
    name = "return_struct",
    srcs = ["return_struct.cc"],
    hdrs = ["return_struct.h"],
)

fhe_cc_library(
    name = "return_struct_tfhe",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
)

cc_binary(
    name = "return_struct_tfhe_testbench",
    srcs = ["return_struct_tfhe_testbench.cc"],
    deps = [
        ":return_struct_tfhe",
        "//transpiler/data:fhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
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
)

cc_binary(
    name = "return_struct_with_inout_tfhe_testbench",
    srcs = ["return_struct_with_inout_tfhe_testbench.cc"],
    deps = [
        ":return_struct_with_inout_tfhe",
        "//transpiler/data:fhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
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
)

cc_binary(
    name = "simple_struct_tfhe_testbench",
    srcs = ["simple_struct_tfhe_testbench.cc"],
    deps = [
        ":simple_struct_tfhe",
        "//transpiler/data:fhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
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
)

cc_binary(
    name = "struct_of_structs_tfhe_testbench",
    srcs = ["struct_of_structs_tfhe_testbench.cc"],
    deps = [
        ":struct_of_structs_tfhe",
        "//transpiler/data:fhe_data",
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
)

cc_binary(
    name = "struct_with_array_tfhe_testbench",
    srcs = ["struct_with_array_tfhe_testbench.cc"],
    deps = [
        ":struct_with_array_tfhe",
        "//transpiler/data:fhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
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
)

cc_binary(
    name = "struct_with_struct_array_tfhe_testbench",
    srcs = ["struct_with_struct_array_tfhe_testbench.cc"],
    deps = [
        ":struct_with_struct_array_tfhe",
        "//transpiler/data:fhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)
