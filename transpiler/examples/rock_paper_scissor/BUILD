load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

cc_library(
    name = "rock_paper_scissor",
    srcs = ["rock_paper_scissor.cc"],
    hdrs = ["rock_paper_scissor.h"],
)

fhe_cc_library(
    name = "rock_paper_scissor_tfhe",
    src = "rock_paper_scissor.cc",
    hdrs = ["rock_paper_scissor.h"],
)

fhe_cc_library(
    name = "rock_paper_scissor_interpreted_tfhe",
    src = "rock_paper_scissor.cc",
    hdrs = ["rock_paper_scissor.h"],
    transpiler_type = "interpreted_tfhe",
)

cc_binary(
    name = "rock_paper_scissor_tfhe_testbench",
    srcs = ["rock_paper_scissor_tfhe_testbench.cc"],
    deps = [
        ":rock_paper_scissor_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "rock_paper_scissor_interpreted_tfhe_testbench",
    srcs = ["rock_paper_scissor_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":rock_paper_scissor_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "rock_paper_scissor_test",
    srcs = ["rock_paper_scissor_test.cc"],
    deps = [
        ":rock_paper_scissor",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)
