load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@rules_python//python:defs.bzl", "py_binary", "py_library")
load("@rules_python//python:defs.bzl", "py_test")
load("//transpiler/jaxite:fhe_py.bzl", "fhe_py_library")

licenses(["notice"])

fhe_py_library(
    name = "add_one_fhe_lib",
    src = "add_one.cc",
    hdrs = ["add_one.h"],
    lut_size = 0,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_py_library(
    name = "add_one_fhe_lib_lut2",
    src = "add_one.cc",
    hdrs = ["add_one.h"],
    lut_size = 2,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_py_library(
    name = "add_one_fhe_lib_lut3",
    src = "add_one.cc",
    hdrs = ["add_one.h"],
    lut_size = 3,
    num_opt_passes = 2,
    optimizer = "yosys",
)

py_library(
    name = "add_one_lib",
    srcs = ["add_one_lib.py"],
    deps = [
        ":add_one_fhe_lib",
        "@transpiler_pip_deps//pypi__jaxite",
    ],
)

py_binary(
    name = "add_one",
    srcs = ["add_one.py"],
    deps = [
        ":add_one_fhe_lib",
        ":add_one_lib",
        "@com_google_absl_py//absl:app",
        "@transpiler_pip_deps//pypi__jaxite",
    ],
)

py_test(
    name = "add_one_test",
    size = "large",
    srcs = [
        "add_one_test.py",
    ],
    python_version = "PY3",
    shard_count = 3,
    srcs_version = "PY3",
    tags = ["nofastbuild"],
    deps = [
        ":add_one_fhe_lib",
        ":add_one_fhe_lib_lut2",
        ":add_one_fhe_lib_lut3",
        ":add_one_lib",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
    ],
)
