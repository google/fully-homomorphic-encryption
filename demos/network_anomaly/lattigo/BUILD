load("@rules_go//go:def.bzl", "go_binary", "go_library")
load("@rules_heir//heir:lattigo.bzl", "heir_lattigo_lib")

package(default_visibility = ["//visibility:public"])

HEIR_OPT_FLAGS = [
    "--annotate-module=backend=lattigo scheme=ckks",
    "--torch-linalg-to-ckks=min-slot-count=8192 greedy-level-budget=15 greedy-modulus-switch-after-mul=true experimental-disable-loop-unroll=true first-mod-bits=30 scaling-mod-bits=24",
    "--scheme-to-lattigo",
]

heir_lattigo_lib(
    name = "kitnet_model_lattigo",
    go_library_name = "kitnet_model_lattigo",
    heir_opt_flags = HEIR_OPT_FLAGS,
    mlir_src = "//demos/network_anomaly/data:torch_kitnet_model_annotated.mlir",
    split_preprocessing = True,
)

go_library(
    name = "utils",
    srcs = ["utils.go"],
    deps = [
        "//demos/common/go/pathutils",
    ],
)

go_binary(
    name = "evaluate_lattigo",
    srcs = [
        "evaluate_lattigo.go",
    ],
    data = [
        "//demos/network_anomaly/data:Mirai_first_batch_32K.bin",
        "//demos/network_anomaly/data:Mirai_labels.csv",
    ],
    deps = [
        ":kitnet_model_lattigo",
        ":kitnet_model_lattigo_utils",
        ":utils",
        "@com_github_tuneinsight_lattigo_v6//core/rlwe",
    ],
)

go_binary(
    name = "evaluate_lattigo_suite",
    srcs = [
        "evaluate_lattigo_suite.go",
    ],
    data = [
        "//demos/network_anomaly/data:Mirai_first_batch_32K.bin",
        "//demos/network_anomaly/data:Mirai_labels.csv",
    ],
    deps = [
        ":kitnet_model_lattigo",
        ":kitnet_model_lattigo_utils",
        ":utils",
        "@com_github_tuneinsight_lattigo_v6//core/rlwe",
    ],
)

heir_lattigo_lib(
    name = "kitnet_model_lattigo_timing",
    extra_srcs = [
        "timing_helper.go",
    ],
    go_library_name = "kitnet_model_lattigo_timing",
    heir_opt_flags = HEIR_OPT_FLAGS,
    mlir_src = "//demos/network_anomaly/data:torch_kitnet_model_annotated.mlir",
    split_preprocessing = True,
    deps = [
        "//demos/common/lattigo/debug",
    ],
)

go_binary(
    name = "evaluate_lattigo_timing",
    srcs = [
        "evaluate_lattigo_timing.go",
    ],
    data = [
        "//demos/network_anomaly/data:Mirai_first_batch_32K.bin",
        "//demos/network_anomaly/data:Mirai_labels.csv",
    ],
    deps = [
        ":kitnet_model_lattigo_timing",
        ":kitnet_model_lattigo_timing_utils",
        ":utils",
    ],
)
