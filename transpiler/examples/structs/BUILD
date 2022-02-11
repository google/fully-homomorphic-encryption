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

fhe_cc_library(
    name = "array_of_array_of_structs_palisade",
    src = "array_of_array_of_structs.cc",
    hdrs = ["array_of_array_of_structs.h"],
    transpiler_type = "palisade",
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
    name = "array_of_array_of_structs_palisade_testbench",
    srcs = ["array_of_array_of_structs_palisade_testbench.cc"],
    deps = [
        ":array_of_array_of_structs_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_library(
    name = "return_struct",
    srcs = ["return_struct.cc"],
    hdrs = ["return_struct.h"],
)

fhe_cc_library(
    name = "return_struct_bool",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    transpiler_type = "bool",
)

fhe_cc_library(
    name = "return_struct_yosys_plaintext",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    transpiler_type = "yosys_plaintext",
)

fhe_cc_library(
    name = "return_struct_tfhe",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
)

fhe_cc_library(
    name = "return_struct_yosys_interpreted_tfhe",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    transpiler_type = "yosys_interpreted_tfhe",
)

fhe_cc_library(
    name = "return_struct_interpreted_tfhe",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    transpiler_type = "interpreted_tfhe",
)

fhe_cc_library(
    name = "return_struct_palisade",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    transpiler_type = "palisade",
)

fhe_cc_library(
    name = "return_struct_interpreted_palisade",
    src = "return_struct.cc",
    hdrs = ["return_struct.h"],
    transpiler_type = "interpreted_palisade",
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
    name = "return_struct_palisade_testbench",
    srcs = ["return_struct_palisade_testbench.cc"],
    deps = [
        ":return_struct_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "return_struct_interpreted_palisade_testbench",
    srcs = ["return_struct_palisade_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_PALISADE"],
    deps = [
        ":return_struct_interpreted_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "return_struct_bool_testbench",
    srcs = ["return_struct_bool_testbench.cc"],
    deps = [
        ":return_struct_bool",
        "//transpiler/data:boolean_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "return_struct_yosys_plaintext_testbench",
    srcs = ["return_struct_bool_testbench.cc"],
    copts = ["-DUSE_YOSYS_PLAINTEXT"],
    deps = [
        ":return_struct_yosys_plaintext",
        "//transpiler/data:boolean_data",
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
)

fhe_cc_library(
    name = "return_struct_with_inout_interpreted_tfhe",
    src = "return_struct_with_inout.cc",
    hdrs = ["return_struct_with_inout.h"],
    transpiler_type = "interpreted_tfhe",
)

fhe_cc_library(
    name = "return_struct_with_inout_palisade",
    src = "return_struct_with_inout.cc",
    hdrs = ["return_struct_with_inout.h"],
    transpiler_type = "palisade",
)

fhe_cc_library(
    name = "return_struct_with_inout_yosys_interpreted_tfhe",
    src = "return_struct_with_inout.cc",
    hdrs = ["return_struct_with_inout.h"],
    transpiler_type = "yosys_interpreted_tfhe",
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
    name = "return_struct_with_inout_palisade_testbench",
    srcs = ["return_struct_with_inout_palisade_testbench.cc"],
    deps = [
        ":return_struct_with_inout_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
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

fhe_cc_library(
    name = "simple_struct_palisade",
    src = "simple_struct.cc",
    hdrs = ["simple_struct.h"],
    transpiler_type = "palisade",
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
    name = "simple_struct_palisade_testbench",
    srcs = ["simple_struct_palisade_testbench.cc"],
    deps = [
        ":simple_struct_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
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

fhe_cc_library(
    name = "struct_of_structs_bool",
    src = "struct_of_structs.cc",
    hdrs = ["struct_of_structs.h"],
    transpiler_type = "bool",
)

fhe_cc_library(
    name = "struct_of_structs_yosys_plaintext",
    src = "struct_of_structs.cc",
    hdrs = ["struct_of_structs.h"],
    transpiler_type = "yosys_plaintext",
)

fhe_cc_library(
    name = "struct_of_structs_yosys_interpreted_tfhe",
    src = "struct_of_structs.cc",
    hdrs = ["struct_of_structs.h"],
    transpiler_type = "yosys_interpreted_tfhe",
)

cc_binary(
    name = "struct_of_structs_bool_testbench",
    srcs = ["struct_of_structs_bool_testbench.cc"],
    deps = [
        ":struct_of_structs_bool",
        "//transpiler/data:boolean_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "struct_of_structs_yosys_plaintext_testbench",
    srcs = ["struct_of_structs_bool_testbench.cc"],
    copts = ["-DUSE_YOSYS_PLAINTEXT"],
    deps = [
        ":struct_of_structs_yosys_plaintext",
        "//transpiler/data:boolean_data",
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
)

fhe_cc_library(
    name = "struct_with_array_interpreted_tfhe",
    src = "struct_with_array.cc",
    hdrs = ["struct_with_array.h"],
    transpiler_type = "interpreted_tfhe",
)

fhe_cc_library(
    name = "struct_with_array_bool",
    src = "struct_with_array.cc",
    hdrs = ["struct_with_array.h"],
    transpiler_type = "bool",
)

fhe_cc_library(
    name = "struct_with_array_yosys_plaintext",
    src = "struct_with_array.cc",
    hdrs = ["struct_with_array.h"],
    transpiler_type = "yosys_plaintext",
)

fhe_cc_library(
    name = "struct_with_array_yosys_interpreted_tfhe",
    src = "struct_with_array.cc",
    hdrs = ["struct_with_array.h"],
    transpiler_type = "yosys_interpreted_tfhe",
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
    name = "struct_with_array_bool_testbench",
    srcs = ["struct_with_array_bool_testbench.cc"],
    deps = [
        ":struct_with_array_bool",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "struct_with_array_yosys_plaintext_testbench",
    srcs = ["struct_with_array_bool_testbench.cc"],
    copts = ["-DUSE_YOSYS_PLAINTEXT"],
    deps = [
        ":struct_with_array_yosys_plaintext",
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
