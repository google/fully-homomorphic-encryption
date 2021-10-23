load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

cc_library(
    name = "is_mail_spam",
    srcs = ["is_mail_spam.cc"],
    hdrs = ["is_mail_spam.h"],
)

fhe_cc_library(
    name = "is_mail_spam_tfhe",
    src = "is_mail_spam.cc",
    hdrs = ["is_mail_spam.h"],
    num_opt_passes = 2,
)

fhe_cc_library(
    name = "is_mail_spam_interpreted_tfhe",
    src = "is_mail_spam.cc",
    hdrs = ["is_mail_spam.h"],
    num_opt_passes = 2,
    transpiler_type = "interpreted_tfhe",
)

fhe_cc_library(
    name = "is_mail_spam_bool",
    src = "is_mail_spam.cc",
    hdrs = ["is_mail_spam.h"],
    num_opt_passes = 2,
    transpiler_type = "bool",
)

cc_binary(
    name = "is_mail_spam_tfhe_testbench",
    srcs = ["is_mail_spam_tfhe_testbench.cc"],
    deps = [
        ":is_mail_spam_tfhe",
        "//transpiler/data:fhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "is_mail_spam_interpreted_tfhe_testbench",
    srcs = ["is_mail_spam_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":is_mail_spam_interpreted_tfhe",
        "//transpiler/data:fhe_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "is_mail_spam_bool_testbench",
    srcs = ["is_mail_spam_bool_testbench.cc"],
    deps = [
        ":is_mail_spam_bool",
        "//transpiler/data:boolean_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_test(
    name = "is_mail_spam_test",
    srcs = ["is_mail_spam_test.cc"],
    deps = [
        ":is_mail_spam",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)
