load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

fhe_cc_library(
    name = "memory",
    src = "memory.cc",
    hdrs = ["memory.h"],
    encryption = "openfhe",
    interpreter = True,
    optimizer = "yosys",
    # num_opt_passes = 2,
    # optimizer = "xls",
)

cc_test(
    name = "memory_test",
    size = "large",
    srcs = ["memory_test.cc"],
    shard_count = 8,
    deps = [
        ":memory",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@openfhe//:binfhe",
    ],
)
