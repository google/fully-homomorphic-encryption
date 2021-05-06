load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

cc_library(
    name = "hangman_api",
    srcs = ["hangman_api.cc"],
    hdrs = ["hangman_api.h"],
    deps = [],
)

cc_binary(
    name = "hangman_client",
    srcs = [
        "hangman_api.h",
        "hangman_client_main.cc",
    ],
    deps = [
        ":hangman_api_tfhe",
        ":hangman_client_lib",
        "//transpiler/data:fhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

fhe_cc_library(
    name = "hangman_api_tfhe",
    src = "hangman_api.cc",
    hdrs = ["hangman_api.h"],
    num_opt_passes = 2,
)

cc_library(
    name = "hangman_client_lib",
    srcs = ["hangman_client.cc"],
    hdrs = ["hangman_client.h"],
    deps = [
        ":hangman_api",
        ":hangman_api_tfhe",
        "//transpiler/data:fhe_data",
    ],
)

cc_test(
    name = "hangman_client_test",
    srcs = ["hangman_client_test.cc"],
    deps = [
        ":hangman_client_lib",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)
