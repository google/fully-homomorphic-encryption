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
    data = [
        "//demos/cc_fraud/data:mlp_fraud_model_sigmoid.pt",
    ],
    deps = [
        ":model",
        "//demos/common/python:export_mlir_utils",
        "//demos/common/python:path_utils",
        requirement("torch"),
    ],
)
