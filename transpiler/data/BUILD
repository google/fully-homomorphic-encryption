load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

cc_library(
    name = "boolean_data",
    hdrs = ["boolean_data.h"],
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/types:span",
    ],
)

cc_library(
    name = "fhe_data",
    hdrs = ["fhe_data.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":boolean_data",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/types:span",
        "@tfhe//:libtfhe",
    ],
)
