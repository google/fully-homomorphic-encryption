load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

BENCHMARK_ARGS = [
]

BENCHMARK_TAGS = [
    "manual",
]

INT_TYPES = [
    "char",
    "int",
]

ENCRYPTIONS = [
    "tfhe",
    "openfhe",
]

OPTIMIZERS = [
    "xls",
    "yosys",
]

BACKEND_DEPS = {
    "openfhe": [
        "//transpiler/data:openfhe_data",
        "//third_party/openfhe/src/binfhe",
    ],
    "tfhe": [
        "//transpiler/data:tfhe_data",
        "//third_party/tfhe",
    ],
}

OPTIMIZER_COPTS = {
    "xls": [],
    "yosys": ["-DUSE_YOSYS_OPTIMIZER"],
}

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
    for encryption in ENCRYPTIONS
    for optimizer in OPTIMIZERS
    for int_type in INT_TYPES
]

# Run with (e.g.):
#
# $ bazel run -c opt --dynamic_mode=off \
#   //transpiler/benchmarks:arithmetic_xls_interpreted_tfhe

[
    cc_test(
        name = "string_cap_cpu_{}_interpreted_{}".format(optimizer, encryption),
        size = "large",
        srcs = ["string_cap_{}.cc".format(encryption)],
        args = BENCHMARK_ARGS,
        copts = OPTIMIZER_COPTS[optimizer],
        tags = BENCHMARK_TAGS,
        deps = BACKEND_DEPS[encryption] + [
            "@com_google_benchmark//:benchmark_main",
            "//transpiler/examples/string_cap:string_cap_{}_{}_interpreted".format(encryption, optimizer),
            "@com_google_xls//xls/common/status:matchers",
        ],
    )
    for encryption in ENCRYPTIONS
    for optimizer in OPTIMIZERS
]

[
    cc_test(
        name = "arithmetic_{}_interpreted_{}".format(optimizer, encryption),
        size = "large",
        srcs = ["arithmetic_{}.cc".format(encryption)],
        args = BENCHMARK_ARGS,
        copts = OPTIMIZER_COPTS[optimizer],
        tags = BENCHMARK_TAGS,
        deps = BACKEND_DEPS[encryption] + [
            "@com_google_benchmark//:benchmark_main",
            "@com_google_xls//xls/common/status:matchers",
        ] + [":add_{}_{}_{}".format(int_type, optimizer, encryption) for int_type in INT_TYPES],
    )
    for encryption in ENCRYPTIONS
    for optimizer in OPTIMIZERS
]
