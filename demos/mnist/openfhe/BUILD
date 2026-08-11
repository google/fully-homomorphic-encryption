load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_heir//heir:openfhe.bzl", "heir_openfhe_lib")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

heir_openfhe_lib(
    name = "mnist_openfhe",
    cc_lib_linkopts = [],
    generated_lib_header = "mnist_openfhe_lib.inc.h",
    heir_opt_flags = [
        "--annotate-module=backend=openfhe scheme=ckks",
        "--torch-linalg-to-ckks=min-slot-count=1024 greedy-modulus-switch-after-mul=true experimental-disable-loop-unroll=true greedy-level-budget=11 first-mod-bits=30 scaling-mod-bits=24",
        "--scheme-to-openfhe",
    ],
    mlir_src = "//demos/mnist/data:mnist.mlir",
    pybind_target_name = "mnist_openfhe_pybind",
    tags = ["nofastbuild"],
)

py_binary(
    name = "evaluate_fhe",
    srcs = ["evaluate_fhe.py"],
    data = [
        "//demos/mnist/data:mnist.npz",
    ],
    main = "evaluate_fhe.py",
    tags = ["nofastbuild"],
    deps = [
        ":mnist_openfhe_pybind",
        "//demos/common/python:path_utils",
        "//demos/mnist/utils:mnist_data",
        "@abseil-py//absl:app",
        "@abseil-py//absl/flags",
        requirement("numpy"),
    ],
)

py_binary(
    name = "evaluate_fhe_suite",
    srcs = ["evaluate_fhe_suite.py"],
    data = [
        "//demos/mnist/data:mnist.npz",
    ],
    main = "evaluate_fhe_suite.py",
    tags = ["nofastbuild"],
    deps = [
        ":mnist_openfhe_pybind",
        "//demos/common/python:path_utils",
        "//demos/mnist/utils:mnist_data",
        "@abseil-py//absl:app",
        "@abseil-py//absl/flags",
        requirement("numpy"),
    ],
)
