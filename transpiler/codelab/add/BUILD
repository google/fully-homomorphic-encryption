load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

fhe_cc_library(
    name = "add_fhe_lib",
    src = "add.cc",
    hdrs = ["add.h"],
    # configurable parameters
    encryption = "openfhe",
    interpreter = True,
    optimizer = "yosys",
)

cc_binary(
    name = "add_openfhe_fhe_demo",
    srcs = [
        "add_openfhe_fhe_demo.cc",
    ],
    deps = [
        ":add_fhe_lib",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/strings",
        "@openfhe//:binfhe",
    ],
)

cc_test(
    name = "add_test",
    srcs = ["add_test.cc"],
    deps = [
        ":add_fhe_lib",
        "//transpiler/data:openfhe_data",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@openfhe//:binfhe",
    ],
)

cc_library(
    name = "add",
    srcs = ["add.cc"],
    hdrs = ["add.h"],
)
