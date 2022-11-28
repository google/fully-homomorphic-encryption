load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

INT_TYPES = [
    "char",
    "int",
]

[
    fhe_cc_library(
        name = "add_{}_{}_{}".format(int_type, optimizer, encryption),
        src = "add_{}.cc".format(int_type),
        hdrs = [],
        encryption = encryption,
        interpreter = True,
        num_opt_passes = 2,
        optimizer = optimizer,
    )
    for int_type in INT_TYPES
    for encryption in [
        "tfhe",
        "openfhe",
    ]
    for optimizer in [
        "xls",
        "yosys",
    ]
]

BENCHMARK_ARGS = [
    #     # Run all the benchmarks (defined with BENCHMARK()) in the test file.
    #     "--benchmark_filter=all",
    #     "--benchmark_min_iters=4",
]

BENCHMARK_TAGS = [
    "manual",
]

# Run with (e.g.):
#
# $ bazel run -c opt --dynamic_mode=off \
#   //transpiler/benchmarks:arithmetic_xls_interpreted_tfhe

cc_test(
    name = "arithmetic_yosys_interpreted_tfhe",
    size = "medium",
    srcs = ["arithmetic_tfhe.cc"],
    args = BENCHMARK_ARGS,
    copts = ["-DUSE_YOSYS_OPTIMIZER"],
    tags = BENCHMARK_TAGS,
    deps = [
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:tfhe_data",
        "@tfhe//:libtfhe",
    ] + [":add_{}_yosys_tfhe".format(int_type) for int_type in INT_TYPES],
)

cc_test(
    name = "arithmetic_xls_interpreted_tfhe",
    size = "medium",
    srcs = ["arithmetic_tfhe.cc"],
    args = BENCHMARK_ARGS,
    tags = BENCHMARK_TAGS,
    deps = [
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:tfhe_data",
        "@tfhe//:libtfhe",
    ] + [":add_{}_xls_tfhe".format(int_type) for int_type in INT_TYPES],
)

cc_test(
    name = "arithmetic_yosys_interpreted_openfhe",
    size = "large",
    srcs = ["arithmetic_openfhe.cc"],
    args = BENCHMARK_ARGS,
    copts = ["-DUSE_YOSYS_OPTIMIZER"],
    tags = BENCHMARK_TAGS,
    deps = [
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:openfhe_data",
        "@openfhe//:binfhe",
    ] + [":add_{}_yosys_openfhe".format(int_type) for int_type in INT_TYPES],
)

cc_test(
    name = "arithmetic_xls_interpreted_openfhe",
    size = "large",
    srcs = ["arithmetic_openfhe.cc"],
    args = BENCHMARK_ARGS,
    tags = BENCHMARK_TAGS,
    deps = [
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:openfhe_data",
        "@openfhe//:binfhe",
    ] + [":add_{}_xls_openfhe".format(int_type) for int_type in INT_TYPES],
)
