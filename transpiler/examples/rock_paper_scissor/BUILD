load("//transpiler:fhe.bzl", "fhe_cc_library")

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
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "rock_paper_scissor_interpreted_tfhe",
    src = "rock_paper_scissor.cc",
    hdrs = ["rock_paper_scissor.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "xls",
)

fhe_cc_library(
    name = "rock_paper_scissor_palisade",
    src = "rock_paper_scissor.cc",
    hdrs = ["rock_paper_scissor.h"],
    encryption = "palisade",
    optimizer = "xls",
)

fhe_cc_library(
    name = "rock_paper_scissor_interpreted_palisade",
    src = "rock_paper_scissor.cc",
    hdrs = ["rock_paper_scissor.h"],
    encryption = "palisade",
    interpreter = True,
    optimizer = "xls",
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

cc_binary(
    name = "rock_paper_scissor_palisade_testbench",
    srcs = ["rock_paper_scissor_palisade_testbench.cc"],
    deps = [
        ":rock_paper_scissor_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "rock_paper_scissor_interpreted_palisade_testbench",
    srcs = ["rock_paper_scissor_palisade_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_PALISADE"],
    deps = [
        ":rock_paper_scissor_interpreted_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
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
