load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

fhe_cc_library(
    name = "string_cap_char_tfhe",
    src = "string_cap_char.cc",
    hdrs = ["string_cap_char.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_binary(
    name = "string_cap_char_tfhe_testbench",
    srcs = [
        "string_cap_char_tfhe_testbench.cc",
    ],
    deps = [
        ":string_cap_char",
        ":string_cap_char_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

fhe_cc_library(
    name = "string_cap_char_interpreted_tfhe",
    src = "string_cap_char.cc",
    hdrs = ["string_cap_char.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_binary(
    name = "string_cap_char_interpreted_tfhe_testbench",
    srcs = [
        "string_cap_char_tfhe_testbench.cc",
    ],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":string_cap_char",
        ":string_cap_char_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_library(
    name = "string_cap_char",
    srcs = ["string_cap_char.cc"],
    hdrs = ["string_cap_char.h"],
)

cc_test(
    name = "string_cap_char_test",
    size = "large",
    srcs = ["string_cap_char_test.cc"],
    deps = [
        ":string_cap_char",
        ":string_cap_char_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@tfhe//:libtfhe",
    ],
)
