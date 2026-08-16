load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "data_utils",
    srcs = ["data_utils.py"],
    deps = [
        requirement("numpy"),
        requirement("pandas"),
        requirement("pyarrow"),
    ],
)

py_binary(
    name = "prep_data",
    srcs = ["prep_data.py"],
    deps = [
        requirement("numpy"),
        requirement("pandas"),
        requirement("pyarrow"),
    ],
)

py_binary(
    name = "encode_data",
    srcs = ["encode_data.py"],
    main = "encode_data.py",
    deps = [
        "//demos/common/python:path_utils",
        requirement("numpy"),
        requirement("pandas"),
        requirement("pyarrow"),
        requirement("scikit-learn"),
    ],
)

py_binary(
    name = "extract_test_rows",
    srcs = ["extract_test_rows.py"],
    main = "extract_test_rows.py",
    deps = [
        "//demos/common/python:path_utils",
        requirement("pandas"),
        requirement("pyarrow"),
    ],
)
