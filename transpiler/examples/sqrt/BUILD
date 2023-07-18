load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler:fhe.bzl", "fhe_cc_library")

# load rust deps

licenses(["notice"])

fhe_cc_library(
    name = "sqrt_tfhe",
    src = "sqrt.cc",
    hdrs = ["sqrt.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "sqrt_yosys_cleartext",
    src = "sqrt.cc",
    hdrs = ["sqrt.h"],
    encryption = "cleartext",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "sqrt_yosys_interpreted_tfhe",
    src = "sqrt.cc",
    hdrs = ["sqrt.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "sqrt_interpreted_tfhe",
    src = "sqrt.cc",
    hdrs = ["sqrt.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "sqrt_openfhe",
    src = "sqrt.cc",
    hdrs = ["sqrt.h"],
    encryption = "openfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "sqrt_interpreted_openfhe",
    src = "sqrt.cc",
    hdrs = ["sqrt.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "sqrt_cleartext",
    src = "sqrt.cc",
    hdrs = ["sqrt.h"],
    encryption = "cleartext",
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_binary(
    name = "sqrt_tfhe_testbench",
    srcs = [
        "sqrt_tfhe_testbench.cc",
    ],
    deps = [
        ":sqrt_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "sqrt_interpreted_tfhe_testbench",
    srcs = [
        "sqrt_tfhe_testbench.cc",
    ],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":sqrt_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "sqrt_yosys_interpreted_tfhe_testbench",
    srcs = [
        "sqrt_tfhe_testbench.cc",
    ],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":sqrt_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "sqrt_openfhe_testbench",
    srcs = [
        "sqrt_openfhe_testbench.cc",
    ],
    deps = [
        ":sqrt_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "sqrt_interpreted_openfhe_testbench",
    srcs = [
        "sqrt_openfhe_testbench.cc",
    ],
    copts = ["-DUSE_INTERPRETED_OPENFHE"],
    deps = [
        ":sqrt_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "sqrt_cleartext_testbench",
    srcs = [
        "sqrt_cleartext_testbench.cc",
    ],
    deps = [
        ":sqrt_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "sqrt_yosys_cleartext_testbench",
    srcs = [
        "sqrt_cleartext_testbench.cc",
    ],
    copts = ["-DUSE_YOSYS_CLEARTEXT"],
    deps = [
        ":sqrt_yosys_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_library(
    name = "sqrt",
    srcs = ["sqrt.cc"],
    hdrs = ["sqrt.h"],
)

cc_test(
    name = "sqrt_test",
    size = "large",
    srcs = ["sqrt_test.cc"],
    shard_count = 8,
    deps = [
        ":sqrt",
        ":sqrt_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@tfhe//:libtfhe",
    ],
)

# sqrt_rs_fhe_lib rules
