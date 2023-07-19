load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler/rust:fhe_rs.bzl", "fhe_rust_library")
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

fhe_rust_library(
    name = "add_one_fhe_lib",
    src = "add_one.cc",
    hdrs = ["add_one.h"],
)

rust_library(
    name = "add_one_lib",
    srcs = ["add_one_lib.rs"],
    deps = [
        ":add_one_fhe_lib",
        "@crate_index//:tfhe",
    ],
)

rust_test(
    name = "add_one_test",
    crate = ":add_one_lib",
)
