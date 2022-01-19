load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

fhe_cc_library(
    name = "calculator_tfhe",
    src = "calculator.cc",
    hdrs = ["calculator.h"],
    num_opt_passes = 2,
)

fhe_cc_library(
    name = "calculator_interpreted_tfhe",
    src = "calculator.cc",
    hdrs = ["calculator.h"],
    num_opt_passes = 2,
    transpiler_type = "interpreted_tfhe",
)

fhe_cc_library(
    name = "calculator_palisade",
    src = "calculator.cc",
    hdrs = ["calculator.h"],
    num_opt_passes = 2,
    transpiler_type = "palisade",
)

fhe_cc_library(
    name = "calculator_interpreted_palisade",
    src = "calculator.cc",
    hdrs = ["calculator.h"],
    num_opt_passes = 2,
    transpiler_type = "interpreted_palisade",
)

cc_binary(
    name = "calculator_tfhe_testbench",
    srcs = ["calculator_tfhe_testbench.cc"],
    deps = [
        ":calculator_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "calculator_interpreted_tfhe_testbench",
    srcs = ["calculator_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":calculator_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/status",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "calculator_palisade_testbench",
    srcs = ["calculator_palisade_testbench.cc"],
    deps = [
        ":calculator_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "calculator_interpreted_palisade_testbench",
    srcs = ["calculator_palisade_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_PALISADE"],
    deps = [
        ":calculator_interpreted_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/status",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_library(
    name = "calculator",
    srcs = ["calculator.cc"],
    hdrs = ["calculator.h"],
    deps = [
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "calculator_testbench",
    srcs = ["calculator_testbench.cc"],
    deps = [
        ":calculator",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@tfhe//:libtfhe",
    ],
)

fhe_cc_library(
    name = "calculator_bool",
    src = "calculator.cc",
    hdrs = ["calculator.h"],
    num_opt_passes = 2,
    transpiler_type = "bool",
)

cc_binary(
    name = "calculator_bool_testbench",
    srcs = ["calculator_bool_testbench.cc"],
    deps = [
        ":calculator_bool",
        "//transpiler/data:boolean_data",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_test(
    name = "calculator_test",
    srcs = ["calculator_test.cc"],
    deps = [
        ":calculator",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)
