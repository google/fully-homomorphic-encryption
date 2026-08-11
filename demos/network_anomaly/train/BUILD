load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "train_pytorch_kitnet",
    srcs = ["train_pytorch_kitnet.py"],
    data = [
        "//demos/network_anomaly/data",
    ],
    deps = [
        "//demos/common/python:path_utils",
        "//demos/network_anomaly/torch:pytorch_kitnet",
        requirement("numpy"),
        requirement("torch"),
    ],
)

py_binary(
    name = "train_50_pytorch_kitnet",
    srcs = ["train_50_pytorch_kitnet.py"],
    data = [
        "//demos/network_anomaly/data",
    ],
    deps = [
        "//demos/common/python:path_utils",
        "//demos/network_anomaly/torch:pytorch_50_kitnet",
        requirement("numpy"),
        requirement("torch"),
    ],
)
