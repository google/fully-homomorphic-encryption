load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

exports_files([
    "debug_reference.json",
    "model_debug.mlir",
    "model_timing.mlir",
])

py_binary(
    name = "generate_debug_reference",
    srcs = ["generate_debug_reference.py"],
    data = [
        "//demos/cc_fraud/data:mlp_fraud_model_sigmoid.pt",
        "//demos/cc_fraud/data:test_rows.csv",
    ],
    deps = [
        "//demos/cc_fraud/torch:model",
        "//demos/common/python:path_utils",
        requirement("numpy"),
        requirement("pandas"),
        requirement("torch"),
    ],
)

py_binary(
    name = "generate_debug_reference_hdr",
    srcs = ["generate_debug_reference_hdr.py"],
)
