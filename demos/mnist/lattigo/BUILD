load("@rules_go//go:def.bzl", "go_binary")
load("@rules_heir//heir:lattigo.bzl", "heir_lattigo_lib")

package(default_visibility = ["//visibility:public"])

HEIR_OPT_FLAGS = [
    "--annotate-module=backend=lattigo scheme=ckks",
    "--torch-linalg-to-ckks=min-slot-count=1024 greedy-modulus-switch-after-mul=true experimental-disable-loop-unroll=true greedy-level-budget=11 first-mod-bits=30 scaling-mod-bits=24",
    "--scheme-to-lattigo",
]

heir_lattigo_lib(
    name = "mnist",
    go_library_name = "mnist",
    heir_opt_flags = HEIR_OPT_FLAGS,
    importpath = "fully_homomorphic_encryption/demos/mnist/lattigo/mnist",
    mlir_src = "//demos/mnist/data:mnist.mlir",
)

go_binary(
    name = "evaluate_fhe",
    srcs = ["evaluate_fhe.go"],
    data = [
        "//demos/mnist/data:mnist.npz",
    ],
    pure = "on",
    deps = [
        ":mnist",
        ":mnist_utils",
        "//demos/common/go/pathutils",
        "//demos/mnist/lattigo/mnist_data",
        "@com_github_tuneinsight_lattigo_v6//core/rlwe",
    ],
)

go_binary(
    name = "evaluate_fhe_suite",
    srcs = ["evaluate_fhe_suite.go"],
    data = [
        "//demos/mnist/data:mnist.npz",
    ],
    pure = "on",
    deps = [
        ":mnist",
        ":mnist_utils",
        "//demos/common/go/pathutils",
        "//demos/mnist/lattigo/mnist_data",
        "@com_github_tuneinsight_lattigo_v6//core/rlwe",
    ],
)

heir_lattigo_lib(
    name = "mnist_timing",
    extra_srcs = ["timing_helper.go"],
    go_library_name = "mnist_timing",
    heir_opt_flags = HEIR_OPT_FLAGS,
    importpath = "fully_homomorphic_encryption/demos/mnist/lattigo/mnist",
    mlir_src = "//demos/mnist/data:mnist_timing.mlir",
    split_preprocessing = True,
    deps = ["//demos/common/lattigo/debug"],
)

go_binary(
    name = "evaluate_fhe_timing",
    srcs = ["evaluate_fhe_timing.go"],
    data = ["//demos/mnist/data:mnist.npz"],
    deps = [
        ":mnist_timing",
        ":mnist_timing_utils",
        "//demos/common/go/pathutils",
        "//demos/mnist/lattigo/mnist_data",
    ],
)
