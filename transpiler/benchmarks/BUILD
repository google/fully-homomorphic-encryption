load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

INT_TYPES = [
    "char",
    "int",
]

[
    fhe_cc_library(
        name = "add_{}_{}_{}".format(int_type, optimizer, backend),
        src = "add_{}.cc".format(int_type),
        hdrs = [],
        backend = backend,
        num_opt_passes = 2,
        optimizer = optimizer,
    )
    for int_type in INT_TYPES
    for backend in [
        "interpreted_tfhe",
        "interpreted_palisade",
    ]
    for optimizer in [
        "xls",
        "yosys",
    ]
]

# Run with (e.g.):
#
# $ blaze run -c opt --dynamic_mode=off \
#   //third_party/fully_homomorphic_encryption/transpiler/benchmarks:arithmetic_xls_interpreted_tfhe \
#   -- \
#       --benchmark_filter=all \
#       --benchmark_min_iters=4

cc_test(
    name = "arithmetic_yosys_interpreted_tfhe",
    srcs = ["arithmetic_tfhe.cc"],
    copts = ["-DUSE_YOSYS_OPTIMIZER"],
    tags = ["manual"],
    deps = [
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:tfhe_data",
        "@tfhe//:libtfhe",
    ] + [":add_{}_yosys_interpreted_tfhe".format(int_type) for int_type in INT_TYPES],
)

cc_test(
    name = "arithmetic_xls_interpreted_tfhe",
    srcs = ["arithmetic_tfhe.cc"],
    tags = ["manual"],
    deps = [
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:tfhe_data",
        "@tfhe//:libtfhe",
    ] + [":add_{}_xls_interpreted_tfhe".format(int_type) for int_type in INT_TYPES],
)

cc_test(
    name = "arithmetic_yosys_interpreted_palisade",
    srcs = ["arithmetic_palisade.cc"],
    copts = ["-DUSE_YOSYS_OPTIMIZER"],
    tags = ["manual"],
    deps = [
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:palisade_data",
        "@palisade//:binfhe",
    ] + [":add_{}_yosys_interpreted_palisade".format(int_type) for int_type in INT_TYPES],
)

cc_test(
    name = "arithmetic_xls_interpreted_palisade",
    srcs = ["arithmetic_palisade.cc"],
    tags = ["manual"],
    deps = [
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:palisade_data",
        "@palisade//:binfhe",
    ] + [":add_{}_xls_interpreted_palisade".format(int_type) for int_type in INT_TYPES],
)
