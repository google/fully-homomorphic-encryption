load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "pytorch_kitnet",
    srcs = ["pytorch_kitnet.py"],
    deps = [
        requirement("numpy"),
        requirement("torch"),
    ],
)

py_library(
    name = "pytorch_50_kitnet",
    srcs = ["pytorch_50_kitnet.py"],
    deps = [
        requirement("numpy"),
        requirement("torch"),
    ],
)

py_binary(
    name = "export_mlir",
    srcs = ["export_mlir.py"],
    data = [
        "//demos/network_anomaly/data",
    ],
    deps = [
        ":pytorch_50_kitnet",
        ":pytorch_kitnet",
        "//demos/common/python:export_mlir_utils",
        "//demos/common/python:path_utils",
        requirement("torch"),
    ],
)
