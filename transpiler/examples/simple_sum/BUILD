load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

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
    num_opt_passes = 2,
)

fhe_cc_library(
    name = "simple_sum_interpreted_tfhe",
    src = "simple_sum.cc",
    hdrs = ["simple_sum.h"],
    num_opt_passes = 2,
    transpiler_type = "interpreted_tfhe",
)

fhe_cc_library(
    name = "simple_sum_palisade",
    src = "simple_sum.cc",
    hdrs = ["simple_sum.h"],
    num_opt_passes = 2,
    transpiler_type = "palisade",
)

fhe_cc_library(
    name = "simple_sum_bool",
    src = "simple_sum.cc",
    hdrs = ["simple_sum.h"],
    num_opt_passes = 2,
    transpiler_type = "bool",
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
    name = "simple_sum_palisade_testbench",
    srcs = ["simple_sum_palisade_testbench.cc"],
    deps = [
        ":simple_sum_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "simple_sum_bool_testbench",
    srcs = ["simple_sum_bool_testbench.cc"],
    deps = [
        ":simple_sum_bool",
        "//transpiler/data:boolean_data",
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
