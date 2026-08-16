load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "evaluate_cleartext",
    srcs = ["evaluate_cleartext.py"],
    main = "evaluate_cleartext.py",
    deps = [
        "//demos/cc_fraud/torch:model",
        "//demos/common/python:path_utils",
        requirement("numpy"),
        requirement("pandas"),
        requirement("pyarrow"),
        requirement("scikit-learn"),
        requirement("torch"),
    ],
)

py_binary(
    name = "estimate_ranges",
    srcs = ["estimate_ranges.py"],
    main = "estimate_ranges.py",
    deps = [
        "//demos/cc_fraud/torch:model",
        "//demos/common/python:path_utils",
        requirement("numpy"),
        requirement("pandas"),
        requirement("pyarrow"),
        requirement("scikit-learn"),
        requirement("torch"),
    ],
)
