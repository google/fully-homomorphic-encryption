load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "evaluate_cleartext",
    srcs = ["evaluate_cleartext.py"],
    data = [
        "//demos/criteo/data:criteohelrm.pth",
    ],
    deps = [
        "//demos/common/python:path_utils",
        "//demos/criteo/torch:model",
        requirement("torch"),
    ],
)
