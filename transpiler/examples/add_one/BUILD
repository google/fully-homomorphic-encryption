load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@rules_python//python:defs.bzl", "py_library")
load("@rules_python//python:defs.bzl", "py_test")
load("@rules_python//python:defs.bzl", "py_binary")
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")
load("//transpiler/jaxite:fhe_py.bzl", "fhe_py_library")
load("//transpiler/rust:fhe_rs.bzl", "fhe_rust_library")

licenses(["notice"])

fhe_rust_library(
    name = "add_one_fhe_rs_lib",
    src = "add_one.cc",
    hdrs = ["add_one.h"],
)

rust_library(
    name = "add_one_rs_lib",
    srcs = ["add_one_lib.rs"],
    rustc_flags = [
        "--cfg",
        "lut",
    ],
    deps = [
        ":add_one_fhe_rs_lib",
        "@crate_index//:tfhe",
    ],
)

fhe_rust_library(
    name = "add_one_gate_fhe_rs_lib",
    src = "add_one.cc",
    hdrs = ["add_one.h"],
    lut_size = 0,
)

rust_library(
    name = "add_one_gate_rs_lib",
    srcs = ["add_one_lib.rs"],
    deps = [
        ":add_one_gate_fhe_rs_lib",
        "@crate_index//:tfhe",
    ],
)

rust_test(
    name = "add_one_gate_rs_test",
    crate = ":add_one_gate_rs_lib",
)

rust_test(
    name = "add_one_rs_test",
    crate = ":add_one_rs_lib",
    rustc_flags = [
        "--cfg",
        "lut",
    ],
)

fhe_py_library(
    name = "add_one_fhe_py_lib",
    src = "add_one.cc",
    hdrs = ["add_one.h"],
    lut_size = 0,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_py_library(
    name = "add_one_fhe_py_lib_lut2",
    src = "add_one.cc",
    hdrs = ["add_one.h"],
    lut_size = 2,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_py_library(
    name = "add_one_fhe_py_lib_lut3",
    src = "add_one.cc",
    hdrs = ["add_one.h"],
    lut_size = 3,
    num_opt_passes = 2,
    optimizer = "yosys",
)

py_library(
    name = "add_one_py_lib",
    srcs = ["add_one_lib.py"],
    deps = [
        ":add_one_fhe_py_lib",
        "@transpiler_pip_deps//pypi__jaxite",
    ],
)

py_binary(
    name = "add_one",
    srcs = ["add_one.py"],
    deps = [
        ":add_one_fhe_py_lib",
        ":add_one_py_lib",
        "@com_google_absl_py//absl:app",
        "@transpiler_pip_deps//pypi__jaxite",
    ],
)

py_test(
    name = "add_one_py_test",
    size = "enormous",
    srcs = [
        "add_one_test.py",
    ],
    main = "add_one_test.py",
    python_version = "PY3",
    shard_count = 3,
    srcs_version = "PY3",
    tags = ["nofastbuild"],
    deps = [
        ":add_one_fhe_py_lib",
        ":add_one_fhe_py_lib_lut2",
        ":add_one_fhe_py_lib_lut3",
        ":add_one_py_lib",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
    ],
)
