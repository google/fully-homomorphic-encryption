load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler:fhe.bzl", "fhe_cc_library")
load("//transpiler/rust:fhe_rs.bzl", "fhe_rust_library")
load("@rules_rust//rust:defs.bzl", "rust_binary", "rust_library", "rust_test")
load("//transpiler/jaxite:fhe_py.bzl", "fhe_py_library")
load("@rules_python//python:defs.bzl", "py_binary")

licenses(["notice"])

fhe_cc_library(
    name = "fibonacci_tfhe",
    src = "fibonacci.cc",
    hdrs = ["fibonacci.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_interpreted_tfhe",
    src = "fibonacci.cc",
    hdrs = ["fibonacci.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_openfhe",
    src = "fibonacci.cc",
    hdrs = ["fibonacci.h"],
    encryption = "openfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_interpreted_openfhe",
    src = "fibonacci.cc",
    hdrs = ["fibonacci.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_sequence_tfhe",
    src = "fibonacci_sequence.cc",
    hdrs = ["fibonacci_sequence.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_sequence_interpreted_tfhe",
    src = "fibonacci_sequence.cc",
    hdrs = ["fibonacci_sequence.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_sequence_cleartext",
    src = "fibonacci_sequence.cc",
    hdrs = ["fibonacci_sequence.h"],
    encryption = "cleartext",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_sequence_openfhe",
    src = "fibonacci_sequence.cc",
    hdrs = ["fibonacci_sequence.h"],
    encryption = "openfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "fibonacci_sequence_interpreted_openfhe",
    src = "fibonacci_sequence.cc",
    hdrs = ["fibonacci_sequence.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_binary(
    name = "fibonacci_tfhe_testbench",
    srcs = ["fibonacci_tfhe_testbench.cc"],
    deps = [
        ":fibonacci_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_interpreted_tfhe_testbench",
    srcs = ["fibonacci_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":fibonacci_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_openfhe_testbench",
    srcs = ["fibonacci_openfhe_testbench.cc"],
    deps = [
        ":fibonacci_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "fibonacci_interpreted_openfhe_testbench",
    srcs = ["fibonacci_openfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_OPENFHE"],
    deps = [
        ":fibonacci_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "fibonacci_sequence_tfhe_testbench",
    srcs = ["fibonacci_sequence_tfhe_testbench.cc"],
    deps = [
        ":fibonacci_sequence_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_sequence_interpreted_tfhe_testbench",
    srcs = ["fibonacci_sequence_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":fibonacci_sequence_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_sequence_cleartext_testbench",
    srcs = ["fibonacci_sequence_cleartext_testbench.cc"],
    deps = [
        ":fibonacci_sequence_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_sequence_openfhe_testbench",
    srcs = ["fibonacci_sequence_openfhe_testbench.cc"],
    deps = [
        ":fibonacci_sequence_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "fibonacci_sequence_interpreted_openfhe_testbench",
    srcs = ["fibonacci_sequence_openfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_OPENFHE"],
    deps = [
        ":fibonacci_sequence_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_library(
    name = "fibonacci",
    srcs = ["fibonacci.cc"],
    hdrs = ["fibonacci.h"],
    deps = [
        "@tfhe//:libtfhe",
    ],
)

cc_library(
    name = "fibonacci_sequence",
    srcs = ["fibonacci_sequence.cc"],
    hdrs = ["fibonacci_sequence.h"],
    deps = [
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "fibonacci_testbench",
    srcs = ["fibonacci_testbench.cc"],
    deps = [
        ":fibonacci",
        ":fibonacci_sequence",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "fibonacci_test",
    size = "large",
    srcs = ["fibonacci_test.cc"],
    deps = [
        ":fibonacci_sequence",
        ":fibonacci_sequence_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@tfhe//:libtfhe",
    ],
)

fhe_rust_library(
    name = "fibonacci_rs_fhe_lib",
    src = "fibonacci.cc",
    hdrs = ["fibonacci.h"],
)

rust_library(
    name = "fibonacci_rs_lib",
    srcs = ["fibonacci_rs_lib.rs"],
    deps = [
        ":fibonacci_rs_fhe_lib",
        "@crate_index//:tfhe",
    ],
)

rust_test(
    name = "fibonacci_rs_test",
    size = "large",
    crate = ":fibonacci_rs_lib",
    tags = ["nofastbuild"],
)

rust_binary(
    name = "fibonacci_rs_main",
    srcs = ["fibonacci_rs_lib.rs"],
    deps = [
        ":fibonacci_rs_fhe_lib",
        "@crate_index//:tfhe",
    ],
)

fhe_py_library(
    name = "fibonacci_fhe_py_lib",
    src = "fibonacci.cc",
    hdrs = ["fibonacci.h"],
    lut_size = 3,
    num_opt_passes = 2,
    optimizer = "yosys",
)

py_binary(
    name = "fibonacci_jaxite_main",
    srcs = ["fibonacci.py"],
    main = "fibonacci.py",
    deps = [
        ":fibonacci_fhe_py_lib",
        "@com_google_absl_py//absl:app",
        "@transpiler_pip_deps//pypi__jaxite",
    ],
)

exports_files(
    [
        "fibonacci.h",
        "fibonacci.cc",
    ],
)
