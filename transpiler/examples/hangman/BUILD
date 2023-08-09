load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler:fhe.bzl", "fhe_cc_library")
load("//transpiler/rust:fhe_rs.bzl", "fhe_rust_library")
load("@rules_rust//rust:defs.bzl", "rust_binary", "rust_library", "rust_test")
load("//transpiler/jaxite:fhe_py.bzl", "fhe_py_library")
load("@rules_python//python:defs.bzl", "py_binary")

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
        "//transpiler/data:tfhe_data",
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
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_library(
    name = "hangman_client_lib",
    srcs = ["hangman_client.cc"],
    hdrs = ["hangman_client.h"],
    deps = [
        ":hangman_api",
        ":hangman_api_tfhe",
        "//transpiler/data:tfhe_data",
    ],
)

cc_test(
    name = "hangman_client_test",
    srcs = ["hangman_client_test.cc"],
    deps = [
        ":hangman_client_lib",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

fhe_rust_library(
    name = "hangman_rs_fhe_lib",
    src = "hangman_api.cc",
    hdrs = ["hangman_api.h"],
)

rust_library(
    name = "hangman_rs_lib",
    srcs = ["hangman_rs_lib.rs"],
    deps = [
        ":hangman_rs_fhe_lib",
        "@crate_index//:tfhe",
    ],
)

rust_test(
    name = "hangman_rs_test",
    size = "large",
    crate = ":hangman_rs_lib",
    tags = ["nofastbuild"],
)

rust_binary(
    name = "hangman_rs_main",
    srcs = ["hangman_rs_lib.rs"],
    deps = [
        ":hangman_rs_fhe_lib",
        "@crate_index//:tfhe",
    ],
)

fhe_py_library(
    name = "hangman_fhe_py_lib",
    src = "hangman_api.cc",
    hdrs = ["hangman_api.h"],
    lut_size = 3,
    num_opt_passes = 2,
    optimizer = "yosys",
)

py_binary(
    name = "hangman_jaxite_main",
    srcs = ["hangman.py"],
    main = "hangman.py",
    deps = [
        ":hangman_fhe_py_lib",
        "@com_google_absl_py//absl:app",
        "@transpiler_pip_deps//pypi__jaxite",
    ],
)
