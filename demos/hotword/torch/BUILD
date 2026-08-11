load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "model",
    srcs = ["model.py"],
    deps = [
        requirement("torch"),
    ],
)

py_binary(
    name = "export_mlir",
    srcs = ["export_mlir.py"],
    main = "export_mlir.py",
    deps = [
        ":model",
        requirement("torch"),
        requirement("torch-mlir"),
    ],
)
