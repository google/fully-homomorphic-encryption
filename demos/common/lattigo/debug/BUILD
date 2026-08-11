load("@rules_go//go:def.bzl", "go_library")

package(default_visibility = ["//visibility:public"])

go_library(
    name = "debug",
    srcs = ["timing_helper.go"],
    importpath = "fully_homomorphic_encryption/demos/common/lattigo/debug",
    deps = [
        "@com_github_tuneinsight_lattigo_v6//core/rlwe",
        "@com_github_tuneinsight_lattigo_v6//schemes/ckks",
    ],
)
