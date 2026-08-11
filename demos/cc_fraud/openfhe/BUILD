load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_heir//heir:openfhe.bzl", "heir_openfhe_lib")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

HEIR_OPT_FLAGS = [
    "--annotate-module=backend=openfhe scheme=ckks",
    "--torch-linalg-to-ckks=min-slot-count=8192 greedy-level-budget=15 greedy-modulus-switch-after-mul=true experimental-disable-loop-unroll=true first-mod-bits=30 scaling-mod-bits=24",
    "--scheme-to-openfhe=scaling-technique-fixed-manual=true",
]

heir_openfhe_lib(
    name = "fraud_model_openfhe_lib",
    cc_lib_linkopts = [],
    cc_lib_target_name = "fraud_model_cc_lib",
    generated_lib_header = "fraud_model.inc.h",
    heir_opt_flags = HEIR_OPT_FLAGS,
    mlir_src = "//demos/cc_fraud/data:model_annotated.mlir",
    pybind_target_name = "fraud_model_pybind",
    tags = ["nofastbuild"],
)

py_binary(
    name = "evaluate_fhe",
    srcs = ["evaluate_fhe.py"],
    data = [
        "//demos/cc_fraud/data:test_rows.csv",
    ],
    main = "evaluate_fhe.py",
    tags = ["nofastbuild"],
    deps = [
        ":fraud_model_pybind",
        "//demos/cc_fraud/utils:data_utils",
        "//demos/common/python:path_utils",
        requirement("numpy"),
        requirement("pandas"),
    ],
)

py_binary(
    name = "evaluate_fhe_suite",
    srcs = ["evaluate_fhe_suite.py"],
    data = [
        "//demos/cc_fraud/data:test_rows.csv",
    ],
    main = "evaluate_fhe_suite.py",
    tags = ["nofastbuild"],
    deps = [
        ":fraud_model_pybind",
        "//demos/cc_fraud/utils:data_utils",
        "//demos/common/python:path_utils",
        requirement("numpy"),
        requirement("pandas"),
    ],
)

heir_openfhe_lib(
    name = "fraud_model_timing_lib",
    cc_lib_linkopts = [],
    cc_lib_target_name = "fraud_model_timing_cc_lib",
    generated_lib_header = "fraud_model_timing.inc.h",
    heir_opt_flags = HEIR_OPT_FLAGS,
    heir_translate_flags = [
        "--openfhe-debug-helper-include-path=demos/common/openfhe/timing_helper.h",
    ],
    mlir_src = "//demos/cc_fraud/debug:model_timing.mlir",
    pybind_target_name = "fraud_model_timing_pybind",
    tags = ["nofastbuild"],
    deps = ["//demos/common/openfhe:timing_helper"],
)

py_binary(
    name = "evaluate_fhe_timing",
    srcs = ["evaluate_fhe_timing.py"],
    data = [
        "//demos/cc_fraud/data:test_rows.csv",
    ],
    main = "evaluate_fhe_timing.py",
    tags = ["nofastbuild"],
    deps = [
        ":fraud_model_timing_pybind",
        "//demos/cc_fraud/utils:data_utils",
        "//demos/common/python:path_utils",
        requirement("numpy"),
        requirement("pandas"),
    ],
)

cc_library(
    name = "debug_helper",
    srcs = ["debug_helper.cpp"],
    hdrs = [
        "debug_helper.h",
        "debug_reference.h",
    ],
    deps = [
        "@openfhe//:core",
        "@openfhe//:pke",
    ],
)

heir_openfhe_lib(
    name = "fraud_model_debug_lib",
    cc_lib_linkopts = [],
    cc_lib_target_name = "fraud_model_debug_cc_lib",
    generated_lib_header = "fraud_model_debug.inc.h",
    heir_opt_flags = HEIR_OPT_FLAGS,
    heir_translate_flags = [
        "--openfhe-debug-helper-include-path=demos/cc_fraud/openfhe/debug_helper.h",
    ],
    mlir_src = "//demos/cc_fraud/debug:model_debug.mlir",
    pybind_target_name = "fraud_model_debug_pybind",
    tags = ["nofastbuild"],
    deps = [":debug_helper"],
)

py_binary(
    name = "evaluate_fhe_debug",
    srcs = ["evaluate_fhe_debug.py"],
    data = [
        "//demos/cc_fraud/data:test_rows.csv",
    ],
    main = "evaluate_fhe_debug.py",
    tags = ["nofastbuild"],
    deps = [
        ":fraud_model_debug_pybind",
        "//demos/cc_fraud/utils:data_utils",
        "//demos/common/python:path_utils",
        requirement("numpy"),
        requirement("pandas"),
    ],
)
