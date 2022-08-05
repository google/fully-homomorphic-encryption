load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

fhe_cc_library(
    name = "ac_int_ops_interpreted_tfhe",
    src = "ac_int_ops.cc",
    hdrs = ["ac_int_ops.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "ac_int_ops_yosys_interpreted_tfhe",
    src = "ac_int_ops.cc",
    hdrs = ["ac_int_ops.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "ac_int_ops_yosys_interpreted_openfhe",
    src = "ac_int_ops.cc",
    hdrs = ["ac_int_ops.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "ac_int_ops_cleartext",
    src = "ac_int_ops.cc",
    hdrs = ["ac_int_ops.h"],
    encryption = "cleartext",
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_binary(
    name = "ac_int_ops_interpreted_tfhe_testbench",
    srcs = [
        "ac_int_ops_tfhe_testbench.cc",
    ],
    copts = [
        "-DUSE_INTERPRETED_TFHE",
    ],
    deps = [
        ":ac_int_ops_interpreted_tfhe",
        "//transpiler/data:fhe_xls_int",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "ac_int_ops_yosys_interpreted_tfhe_testbench",
    srcs = [
        "ac_int_ops_tfhe_testbench.cc",
    ],
    copts = [
        "-DUSE_YOSYS_INTERPRETED_TFHE",
    ],
    deps = [
        ":ac_int_ops_yosys_interpreted_tfhe",
        "//transpiler/data:fhe_xls_int",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "ac_int_ops_yosys_interpreted_openfhe_testbench",
    srcs = [
        "ac_int_ops_openfhe_testbench.cc",
    ],
    deps = [
        ":ac_int_ops_yosys_interpreted_openfhe",
        "//transpiler/data:fhe_xls_int",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "ac_int_ops_cleartext_testbench",
    srcs = [
        "ac_int_ops_cleartext_testbench.cc",
    ],
    deps = [
        ":ac_int_ops_cleartext",
        "//transpiler/data:cleartext_data",
        "//transpiler/data:fhe_xls_int",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)
