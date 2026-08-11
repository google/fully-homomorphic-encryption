load("@rules_go//go:def.bzl", "go_binary")
load("@rules_heir//heir:lattigo.bzl", "heir_lattigo_lib")

package(default_visibility = ["//visibility:public"])

HEIR_OPT_FLAGS = [
    "--annotate-module=backend=lattigo scheme=ckks",
    "--torch-linalg-to-ckks=min-slot-count=8192 greedy-level-budget=15 greedy-modulus-switch-after-mul=true experimental-disable-loop-unroll=true first-mod-bits=30 scaling-mod-bits=24",
    "--scheme-to-lattigo",
]

heir_lattigo_lib(
    name = "fraud_model_lattigo",
    go_library_name = "fraud_model_lattigo",
    heir_opt_flags = HEIR_OPT_FLAGS,
    importpath = "fully_homomorphic_encryption/demos/cc_fraud/lattigo/fraud_model_lattigo",
    mlir_src = "//demos/cc_fraud/data:model_annotated.mlir",
    split_preprocessing = True,
)

go_binary(
    name = "evaluate_fhe",
    srcs = [
        "evaluate_fhe.go",
        "utils.go",
    ],
    data = [
        "//demos/cc_fraud/data:test_rows.csv",
    ],
    pure = "on",
    deps = [
        ":fraud_model_lattigo",
        ":fraud_model_lattigo_utils",
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
        "//demos/cc_fraud/data:test_rows.csv",
    ],
    pure = "on",
    deps = [
        ":fraud_model_lattigo",
        ":fraud_model_lattigo_utils",
        "//demos/common/go/pathutils",
        "@com_github_tuneinsight_lattigo_v6//core/rlwe",
    ],
)

heir_lattigo_lib(
    name = "fraud_model_lattigo_timing",
    extra_srcs = [
        "timing_helper.go",
    ],
    go_library_name = "fraud_model_lattigo_timing",
    heir_opt_flags = HEIR_OPT_FLAGS,
    importpath = "fully_homomorphic_encryption/demos/cc_fraud/lattigo/fraud_model_lattigo_timing",
    mlir_src = "//demos/cc_fraud/debug:model_timing.mlir",
    split_preprocessing = True,
    deps = [
        "//demos/common/lattigo/debug",
    ],
)

go_binary(
    name = "evaluate_fhe_timing",
    srcs = [
        "evaluate_fhe_timing.go",
        "utils.go",
    ],
    data = [
        "//demos/cc_fraud/data:test_rows.csv",
    ],
    pure = "on",
    deps = [
        ":fraud_model_lattigo_timing",
        ":fraud_model_lattigo_timing_utils",
        "//demos/common/go/pathutils",
    ],
)

heir_lattigo_lib(
    name = "fraud_model_lattigo_debug",
    extra_srcs = [
        "debug_helper.go",
    ],
    go_library_name = "fraud_model_lattigo_debug",
    heir_opt_flags = HEIR_OPT_FLAGS,
    importpath = "fully_homomorphic_encryption/demos/cc_fraud/lattigo/fraud_model_lattigo_debug",
    mlir_src = "//demos/cc_fraud/debug:model_debug.mlir",
    split_preprocessing = True,
    deps = [
        "//demos/common/go/pathutils",
    ],
)

go_binary(
    name = "evaluate_fhe_debug",
    srcs = [
        "evaluate_fhe_debug.go",
        "utils.go",
    ],
    data = [
        "//demos/cc_fraud/data:test_rows.csv",
        "//demos/cc_fraud/debug:debug_reference.json",
    ],
    pure = "on",
    deps = [
        ":fraud_model_lattigo_debug",
        ":fraud_model_lattigo_debug_utils",
        "//demos/common/go/pathutils",
    ],
)
