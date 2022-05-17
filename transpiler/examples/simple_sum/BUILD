load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

cc_library(
    name = "simple_sum",
    srcs = ["simple_sum.cc"],
    hdrs = ["simple_sum.h"],
)

fhe_cc_library(
    name = "simple_sum_tfhe",
    src = "simple_sum.cc",
    hdrs = ["simple_sum.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "simple_sum_interpreted_tfhe",
    src = "simple_sum.cc",
    hdrs = ["simple_sum.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "simple_sum_openfhe",
    src = "simple_sum.cc",
    hdrs = ["simple_sum.h"],
    encryption = "openfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "simple_sum_interpreted_openfhe",
    src = "simple_sum.cc",
    hdrs = ["simple_sum.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "simple_sum_cleartext",
    src = "simple_sum.cc",
    hdrs = ["simple_sum.h"],
    encryption = "cleartext",
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_binary(
    name = "simple_sum_tfhe_testbench",
    srcs = ["simple_sum_tfhe_testbench.cc"],
    deps = [
        ":simple_sum_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "simple_sum_interpreted_tfhe_testbench",
    srcs = ["simple_sum_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":simple_sum_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "simple_sum_openfhe_testbench",
    srcs = ["simple_sum_openfhe_testbench.cc"],
    deps = [
        ":simple_sum_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "simple_sum_interpreted_openfhe_testbench",
    srcs = ["simple_sum_openfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_OPENFHE"],
    deps = [
        ":simple_sum_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "simple_sum_cleartext_testbench",
    srcs = ["simple_sum_cleartext_testbench.cc"],
    deps = [
        ":simple_sum_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_test(
    name = "simple_sum_test",
    srcs = ["simple_sum_test.cc"],
    deps = [
        ":simple_sum",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)
