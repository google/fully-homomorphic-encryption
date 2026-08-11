load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library", "py_test")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "mnist_data",
    srcs = ["mnist_data.py"],
    deps = [
        requirement("numpy"),
    ],
)

py_test(
    name = "mnist_data_test",
    srcs = ["mnist_data_test.py"],
    data = [
        "//demos/mnist/data:mnist.npz",
    ],
    deps = [
        ":mnist_data",
        requirement("absl-py"),
    ],
)
