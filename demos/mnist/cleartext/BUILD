load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "evaluate_cleartext",
    srcs = ["evaluate_cleartext.py"],
    data = [
        "//demos/mnist/data:mlp_model.pth",
        "//demos/mnist/data:mnist.npz",
    ],
    main = "evaluate_cleartext.py",
    deps = [
        "//demos/common/python:path_utils",
        "//demos/mnist/torch:model",
        "//demos/mnist/utils:mnist_data",
        requirement("numpy"),
        requirement("torch"),
    ],
)

py_binary(
    name = "evaluate_cleartext_suite",
    srcs = ["evaluate_cleartext_suite.py"],
    data = [
        "//demos/mnist/data:mlp_model.pth",
        "//demos/mnist/data:mnist.npz",
    ],
    main = "evaluate_cleartext_suite.py",
    deps = [
        "//demos/common/python:path_utils",
        "//demos/mnist/torch:model",
        "//demos/mnist/utils:mnist_data",
        requirement("numpy"),
        requirement("torch"),
    ],
)
