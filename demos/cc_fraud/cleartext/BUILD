load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "evaluate_cleartext",
    srcs = ["evaluate_cleartext.py"],
    data = [
        "//demos/cc_fraud/data:feature_cols.pkl",
        "//demos/cc_fraud/data:mlp_fraud_model_sigmoid.pt",
        "//demos/cc_fraud/data:test_rows.csv",
    ],
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
