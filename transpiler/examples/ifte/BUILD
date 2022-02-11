load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

fhe_cc_library(
    name = "ifte_yosys_plaintext",
    src = "ifte.cc",
    hdrs = ["ifte.h"],
    backend = "cleartext",
    optimizer = "yosys",
)

fhe_cc_library(
    name = "ifte_yosys_interpreted_tfhe",
    src = "ifte.cc",
    hdrs = ["ifte.h"],
    backend = "interpreted_tfhe",
    optimizer = "yosys",
)

fhe_cc_library(
    name = "ifte_bool",
    src = "ifte.cc",
    hdrs = ["ifte.h"],
    backend = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "ifte_tfhe",
    src = "ifte.cc",
    hdrs = ["ifte.h"],
)

cc_binary(
    name = "ifte_bool_testbench",
    srcs = ["ifte_bool_testbench.cc"],
    deps = [
        ":ifte_bool",
        "//transpiler/data:boolean_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "ifte_yosys_plaintext_testbench",
    srcs = ["ifte_bool_testbench.cc"],
    copts = ["-DUSE_YOSYS_PLAINTEXT"],
    deps = [
        ":ifte_yosys_plaintext",
        "//transpiler/data:boolean_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "ifte_tfhe_testbench",
    srcs = ["ifte_tfhe_testbench.cc"],
    deps = [
        ":ifte_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "ifte_yosys_interpreted_tfhe_testbench",
    srcs = ["ifte_tfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":ifte_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)
