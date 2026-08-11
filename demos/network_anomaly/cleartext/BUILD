load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "evaluate_single",
    srcs = ["evaluate_single.py"],
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
    name = "evaluate_suite",
    srcs = ["evaluate_suite.py"],
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
    name = "evaluate_50_single",
    srcs = ["evaluate_50_single.py"],
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

py_binary(
    name = "evaluate_50_suite",
    srcs = ["evaluate_50_suite.py"],
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
