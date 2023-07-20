# Hello World example

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@rules_rust//rust:defs.bzl", "rust_binary", "rust_library")

package(
    default_applicable_licenses = ["@com_google_fully_homomorphic_encryption//:license"],
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

rust_library(
    name = "hello_world_fhe_lib",
    srcs = ["hello_world_fhe_lib.rs"],
    disable_pipelining = True,
    tags = [
        "manual",
        "notap",
    ],
    deps = [
        "@crate_index//:rayon",
        "@crate_index//:tfhe",
    ],
)

rust_binary(
    name = "hello_world_testbench",
    srcs = [
        "hello_world_testbench.rs",
    ],
    tags = [
        "manual",
        "notap",
    ],
    deps = [
        ":hello_world_fhe_lib",
        "@crate_index//:rayon",
        "@crate_index//:tfhe",
    ],
)
