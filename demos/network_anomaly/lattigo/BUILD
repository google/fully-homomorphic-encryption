load("@rules_go//go:def.bzl", "go_binary", "go_library")
load("@rules_heir//heir:lattigo.bzl", "heir_lattigo_lib")

package(default_visibility = ["//visibility:public"])

HEIR_OPT_FLAGS = [
    "--annotate-module=backend=lattigo scheme=ckks",
    "--torch-linalg-to-ckks=min-slot-count=8192 greedy-level-budget=15 greedy-modulus-switch-after-mul=true experimental-disable-loop-unroll=true first-mod-bits=30 scaling-mod-bits=24",
    "--scheme-to-lattigo",
]

heir_lattigo_lib(
    name = "anomaly_model_lattigo",
    go_library_name = "anomaly_model_lattigo",
    heir_opt_flags = HEIR_OPT_FLAGS,
    importpath = "fully_homomorphic_encryption/demos/network_anomaly/lattigo/anomaly_model_lattigo",
    mlir_src = "//demos/network_anomaly/data:torch_kitnet_model_annotated.mlir",
    split_preprocessing = True,
)

go_library(
    name = "utils",
    srcs = ["utils.go"],
    importpath = "fully_homomorphic_encryption/demos/network_anomaly/lattigo/utils",
    deps = [
        "//demos/common/go/pathutils",
    ],
)

go_binary(
    name = "evaluate_fhe",
    srcs = [
        "evaluate_fhe.go",
    ],
    data = [
        "//demos/network_anomaly/data:Mirai_first_batch_32K.bin",
        "//demos/network_anomaly/data:Mirai_labels.csv",
    ],
    pure = "on",
    deps = [
        ":anomaly_model_lattigo",
        ":anomaly_model_lattigo_utils",
        ":utils",
        "@com_github_tuneinsight_lattigo_v6//core/rlwe",
    ],
)

go_binary(
    name = "evaluate_fhe_suite",
    srcs = [
        "evaluate_fhe_suite.go",
    ],
    data = [
        "//demos/network_anomaly/data:Mirai_first_batch_32K.bin",
        "//demos/network_anomaly/data:Mirai_labels.csv",
    ],
    pure = "on",
    deps = [
        ":anomaly_model_lattigo",
        ":anomaly_model_lattigo_utils",
        ":utils",
        "@com_github_tuneinsight_lattigo_v6//core/rlwe",
    ],
)

go_binary(
    name = "evaluate_fhe_timing",
    srcs = [
        "evaluate_fhe_timing.go",
    ],
    data = [
        "//demos/network_anomaly/data:Mirai_first_batch_32K.bin",
        "//demos/network_anomaly/data:Mirai_labels.csv",
    ],
    pure = "on",
    deps = [
        ":anomaly_model_lattigo",
        ":anomaly_model_lattigo_utils",
        ":utils",
    ],
)
