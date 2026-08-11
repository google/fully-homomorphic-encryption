load("@rules_go//go:def.bzl", "go_binary")
load("@rules_heir//heir:lattigo.bzl", "heir_lattigo_lib")

package(default_visibility = ["//visibility:public"])

heir_lattigo_lib(
    name = "criteo",
    go_library_name = "criteo",
    heir_opt_flags = [
        "--annotate-module=backend=lattigo scheme=ckks",
        "--torch-linalg-to-ckks=min-slot-count=32768 greedy-modulus-switch-after-mul=true experimental-disable-loop-unroll=true greedy-level-budget=11 first-mod-bits=55 scaling-mod-bits=30",
        "--scheme-to-lattigo",
    ],
    importpath = "fully_homomorphic_encryption/demos/criteo/lattigo/criteo",
    mlir_src = "//demos/criteo/data:criteohelrm_torch.mlir",
)

go_binary(
    name = "evaluate_fhe",
    srcs = ["evaluate_fhe.go"],
    pure = "on",
    deps = [
        ":criteo",
        ":criteo_utils",
    ],
)
