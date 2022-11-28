load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

fhe_cc_library(
    name = "inc3d_tfhe",
    src = "inc3d.cc",
    hdrs = ["sum3d.h"],
    encryption = "tfhe",
    #    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "inc3d_yosys_interpreted_tfhe",
    src = "inc3d.cc",
    hdrs = ["sum3d.h"],
    encryption = "tfhe",
    #    num_opt_passes = 2,
    interpreter = True,
    optimizer = "yosys",
)

cc_library(
    name = "sum3d",
    srcs = ["sum3d.cc"],
    hdrs = ["sum3d.h"],
)

fhe_cc_library(
    name = "sum3d_tfhe",
    src = "sum3d.cc",
    hdrs = ["sum3d.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "sum3d_interpreted_tfhe",
    src = "sum3d.cc",
    hdrs = ["sum3d.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "sum3d_yosys_interpreted_tfhe",
    src = "sum3d.cc",
    hdrs = ["sum3d.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "sum3d_openfhe",
    src = "sum3d.cc",
    hdrs = ["sum3d.h"],
    encryption = "openfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "sum3d_interpreted_openfhe",
    src = "sum3d.cc",
    hdrs = ["sum3d.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "sum3d_yosys_interpreted_openfhe",
    src = "sum3d.cc",
    hdrs = ["sum3d.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "sum3d_cleartext",
    src = "sum3d.cc",
    hdrs = ["sum3d.h"],
    encryption = "cleartext",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "sum3d_yosys_interpreted_cleartext",
    src = "sum3d.cc",
    hdrs = ["sum3d.h"],
    encryption = "cleartext",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

cc_binary(
    name = "sum3d_tfhe_testbench",
    srcs = ["sum3d_tfhe_testbench.cc"],
    deps = [
        ":sum3d_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "sum3d_interpreted_tfhe_testbench",
    srcs = ["sum3d_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":sum3d_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "sum3d_yosys_interpreted_tfhe_testbench",
    srcs = ["sum3d_tfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":sum3d_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "sum3d_openfhe_testbench",
    srcs = ["sum3d_openfhe_testbench.cc"],
    deps = [
        ":sum3d_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "sum3d_interpreted_openfhe_testbench",
    srcs = ["sum3d_openfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_OPENFHE"],
    deps = [
        ":sum3d_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "sum3d_yosys_interpreted_openfhe_testbench",
    srcs = ["sum3d_openfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_OPENFHE"],
    deps = [
        ":sum3d_yosys_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "sum3d_cleartext_testbench",
    srcs = ["sum3d_cleartext_testbench.cc"],
    deps = [
        ":sum3d_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "sum3d_yosys_interpreted_cleartext_testbench",
    srcs = ["sum3d_cleartext_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_CLEARTEXT"],
    deps = [
        ":sum3d_yosys_interpreted_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)
