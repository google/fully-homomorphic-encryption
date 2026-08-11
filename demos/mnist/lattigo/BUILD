load("@rules_go//go:def.bzl", "go_binary")
load("@rules_heir//heir:lattigo.bzl", "heir_lattigo_lib")

package(default_visibility = ["//visibility:public"])

heir_lattigo_lib(
    name = "mnist",
    go_library_name = "mnist",
    heir_opt_flags = [
        "--annotate-module=backend=lattigo scheme=ckks",
        "--torch-linalg-to-ckks=min-slot-count=1024 greedy-modulus-switch-after-mul=true experimental-disable-loop-unroll=true greedy-level-budget=20",
        "--scheme-to-lattigo",
    ],
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
