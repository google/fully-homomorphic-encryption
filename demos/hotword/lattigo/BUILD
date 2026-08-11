load("@rules_go//go:def.bzl", "go_binary")
load("@rules_heir//heir:lattigo.bzl", "heir_lattigo_lib")

package(default_visibility = ["//visibility:public"])

HEIR_OPT_FLAGS = [
    "--annotate-module=backend=lattigo scheme=ckks",
    "--torch-linalg-to-ckks=min-slot-count=4096 greedy-modulus-switch-after-mul=true experimental-disable-loop-unroll=true greedy-level-budget=11 first-mod-bits=55 scaling-mod-bits=30",
    "--scheme-to-lattigo",
]

heir_lattigo_lib(
    name = "hotword_lattigo",
    go_library_name = "hotword_lattigo",
    heir_opt_flags = HEIR_OPT_FLAGS,
    importpath = "fully_homomorphic_encryption/demos/hotword/lattigo/hotword_lattigo",
    mlir_src = "//demos/hotword/data:hotword.mlir",
    split_preprocessing = True,
)

heir_lattigo_lib(
    name = "hotword_lattigo_timing",
    extra_srcs = [
        "timing_helper.go",
    ],
    go_library_name = "hotwordlattigotiming",
    heir_opt_flags = HEIR_OPT_FLAGS,
    importpath = "fully_homomorphic_encryption/demos/hotword/lattigo/hotwordlattigotiming",
    mlir_src = "//demos/hotword/data:hotword_timing.mlir",
    split_preprocessing = True,
    deps = [
        "//demos/common/lattigo/debug",
    ],
)

go_binary(
    name = "evaluate_fhe",
    srcs = [
        "evaluate_fhe.go",
        "utils.go",
    ],
    data = [
        "//demos/hotword/data:test_data.npz",
    ],
    pure = "on",
    deps = [
        ":hotword_lattigo",
        ":hotword_lattigo_utils",
        "//demos/common/go/pathutils",
    ],
)

go_binary(
    name = "evaluate_fhe_suite",
    srcs = [
        "evaluate_fhe_suite.go",
        "utils.go",
    ],
    data = [
        "//demos/hotword/data:test_data.npz",
    ],
    pure = "on",
    deps = [
        ":hotword_lattigo",
        ":hotword_lattigo_utils",
        "//demos/common/go/pathutils",
        "@com_github_tuneinsight_lattigo_v6//core/rlwe",
    ],
)

go_binary(
    name = "evaluate_fhe_timing",
    srcs = [
        "evaluate_fhe_timing.go",
        "utils.go",
    ],
    data = [
        "//demos/hotword/data:test_data.npz",
    ],
    pure = "on",
    deps = [
        ":hotwordlattigotiming",
        ":hotwordlattigotiming_utils",
        "//demos/common/go/pathutils",
    ],
)
