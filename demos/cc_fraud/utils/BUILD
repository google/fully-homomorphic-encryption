load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "data_utils",
    srcs = ["data_utils.py"],
    deps = [
        requirement("numpy"),
        requirement("pandas"),
    ],
)

py_binary(
    name = "prep_data",
    srcs = ["prep_data.py"],
    deps = [
        requirement("numpy"),
        requirement("pandas"),
    ],
)

py_binary(
    name = "encode_data",
    srcs = ["encode_data.py"],
    data = [
        "//demos/cc_fraud/data:encoder_mappings.json",
        "//demos/cc_fraud/data:feature_cols.pkl",
        "//demos/cc_fraud/data:scaler.pkl",
    ],
    deps = [
        "//demos/common/python:path_utils",
        requirement("numpy"),
        requirement("pandas"),
    ],
)

py_binary(
    name = "extract_test_rows",
    srcs = ["extract_test_rows.py"],
    data = [
        "//demos/cc_fraud/data:feature_cols.pkl",
    ],
    deps = [
        "//demos/common/python:path_utils",
        requirement("pandas"),
    ],
)
