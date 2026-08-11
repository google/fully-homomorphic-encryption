load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary", "py_library", "py_test")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "model",
    srcs = ["model.py"],
    deps = [
        requirement("torch"),
    ],
)

py_binary(
    name = "test_helrm",
    srcs = ["test_helrm.py"],
    deps = [
        ":model",
        requirement("torch"),
    ],
)

py_test(
    name = "criteo_inference_test",
    srcs = ["criteo_inference_test.py"],
    data = [
        "//demos/criteo/data:criteo_data",
    ],
    deps = [
        ":model",
        requirement("absl-py"),
        requirement("torch"),
    ],
)
