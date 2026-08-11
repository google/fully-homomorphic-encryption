load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "sample_data",
    srcs = ["sample_data.py"],
    deps = [
        requirement("torch"),
    ],
)

py_binary(
    name = "create_dataset",
    srcs = ["create_dataset.py"],
    deps = [
        requirement("numpy"),
    ],
)

py_binary(
    name = "convert_criteo_parquet",
    srcs = ["convert_criteo_parquet.py"],
    deps = [
        requirement("pandas"),
        requirement("pyarrow"),
    ],
)
