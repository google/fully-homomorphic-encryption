load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

fhe_cc_library(
    name = "fibonacci_tfhe",
    src = "fibonacci.cc",
    hdrs = ["fibonacci.h"],
    num_opt_passes = 2,
)

fhe_cc_library(
    name = "fibonacci_interpreted_tfhe",
    src = "fibonacci.cc",
    hdrs = ["fibonacci.h"],
    backend = "interpreted_tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_palisade",
    src = "fibonacci.cc",
    hdrs = ["fibonacci.h"],
    backend = "palisade",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_interpreted_palisade",
    src = "fibonacci.cc",
    hdrs = ["fibonacci.h"],
    backend = "interpreted_palisade",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_sequence_tfhe",
    src = "fibonacci_sequence.cc",
    hdrs = ["fibonacci_sequence.h"],
    num_opt_passes = 2,
)

fhe_cc_library(
    name = "fibonacci_sequence_interpreted_tfhe",
    src = "fibonacci_sequence.cc",
    hdrs = ["fibonacci_sequence.h"],
    backend = "interpreted_tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_sequence_cleartext",
    src = "fibonacci_sequence.cc",
    hdrs = ["fibonacci_sequence.h"],
    backend = "cleartext",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_sequence_palisade",
    src = "fibonacci_sequence.cc",
    hdrs = ["fibonacci_sequence.h"],
    backend = "palisade",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_sequence_interpreted_palisade",
    src = "fibonacci_sequence.cc",
    hdrs = ["fibonacci_sequence.h"],
    backend = "interpreted_palisade",
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_binary(
    name = "fibonacci_tfhe_testbench",
    srcs = ["fibonacci_tfhe_testbench.cc"],
    deps = [
        ":fibonacci_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_interpreted_tfhe_testbench",
    srcs = ["fibonacci_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":fibonacci_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_palisade_testbench",
    srcs = ["fibonacci_palisade_testbench.cc"],
    deps = [
        ":fibonacci_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "fibonacci_interpreted_palisade_testbench",
    srcs = ["fibonacci_palisade_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_PALISADE"],
    deps = [
        ":fibonacci_interpreted_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "fibonacci_sequence_tfhe_testbench",
    srcs = ["fibonacci_sequence_tfhe_testbench.cc"],
    deps = [
        ":fibonacci_sequence_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_sequence_interpreted_tfhe_testbench",
    srcs = ["fibonacci_sequence_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":fibonacci_sequence_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_sequence_cleartext_testbench",
    srcs = ["fibonacci_sequence_cleartext_testbench.cc"],
    deps = [
        ":fibonacci_sequence_cleartext",
        "//transpiler/data:boolean_data",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_sequence_palisade_testbench",
    srcs = ["fibonacci_sequence_palisade_testbench.cc"],
    deps = [
        ":fibonacci_sequence_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "fibonacci_sequence_interpreted_palisade_testbench",
    srcs = ["fibonacci_sequence_palisade_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_PALISADE"],
    deps = [
        ":fibonacci_sequence_interpreted_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_library(
    name = "fibonacci",
    srcs = ["fibonacci.cc"],
    hdrs = ["fibonacci.h"],
    deps = [
        "@tfhe//:libtfhe",
    ],
)

cc_library(
    name = "fibonacci_sequence",
    srcs = ["fibonacci_sequence.cc"],
    hdrs = ["fibonacci_sequence.h"],
    deps = [
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_testbench",
    srcs = ["fibonacci_testbench.cc"],
    deps = [
        ":fibonacci",
        ":fibonacci_sequence",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "fibonacci_test",
    size = "large",
    srcs = ["fibonacci_test.cc"],
    deps = [
        ":fibonacci_sequence",
        ":fibonacci_sequence_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
        "@tfhe//:libtfhe",
    ],
)
