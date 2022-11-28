load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

fhe_cc_library(
    name = "calculator_tfhe",
    src = "calculator.cc",
    hdrs = ["calculator.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "calculator_interpreted_tfhe",
    src = "calculator.cc",
    hdrs = ["calculator.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "calculator_openfhe",
    src = "calculator.cc",
    hdrs = ["calculator.h"],
    encryption = "openfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "calculator_interpreted_openfhe",
    src = "calculator.cc",
    hdrs = ["calculator.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
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
    name = "calculator_openfhe_testbench",
    srcs = ["calculator_openfhe_testbench.cc"],
    deps = [
        ":calculator_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "calculator_interpreted_openfhe_testbench",
    srcs = ["calculator_openfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_OPENFHE"],
    deps = [
        ":calculator_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/status",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
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
    name = "calculator_cleartext",
    src = "calculator.cc",
    hdrs = ["calculator.h"],
    encryption = "cleartext",
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_binary(
    name = "calculator_cleartext_testbench",
    srcs = ["calculator_cleartext_testbench.cc"],
    deps = [
        ":calculator_cleartext",
        "//transpiler/data:cleartext_data",
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
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)
