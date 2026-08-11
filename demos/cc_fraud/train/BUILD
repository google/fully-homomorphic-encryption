load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "train",
    srcs = ["train.py"],
    data = [
        "//demos/cc_fraud/data:feature_cols.pkl",
    ],
    deps = [
        "//demos/cc_fraud/torch:model",
        "//demos/common/python:path_utils",
        requirement("numpy"),
        requirement("pandas"),
        requirement("scikit-learn"),
        requirement("torch"),
    ],
)
