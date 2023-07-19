load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

# Build rules for the template-based string reversal example.
load("//transpiler:fhe.bzl", "fhe_cc_library")
load("//transpiler/rust:fhe_rs.bzl", "fhe_rust_library")
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

licenses(["notice"])

fhe_cc_library(
    name = "mul8_tfhe",
    src = "mul8.cc",
    hdrs = [
        "mul.h",
        "mul8.h",
    ],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "mul16_tfhe",
    src = "mul16.cc",
    hdrs = [
        "mul.h",
        "mul16.h",
    ],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "mul8_interpreted_tfhe",
    src = "mul8.cc",
    hdrs = [
        "mul.h",
        "mul8.h",
    ],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "mul16_interpreted_tfhe",
    src = "mul16.cc",
    hdrs = [
        "mul.h",
        "mul16.h",
    ],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_binary(
    name = "mul_tfhe_testbench",
    srcs = ["mul_tfhe_testbench.cc"],
    deps = [
        ":mul16_tfhe",
        ":mul8_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "mul_interpreted_tfhe_testbench",
    srcs = ["mul_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":mul16_interpreted_tfhe",
        ":mul8_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

fhe_rust_library(
    name = "mul_rs_fhe_lib",
    src = "mul16.cc",
    hdrs = [
        "mul.h",
        "mul16.h",
    ],
)

rust_library(
    name = "mul_rs_lib",
    srcs = ["mul_rs_lib.rs"],
    deps = [
        ":mul_rs_fhe_lib",
        "@crate_index//:tfhe",
    ],
)

rust_test(
    name = "mul_rs_test",
    size = "large",
    crate = ":mul_rs_lib",
    tags = ["nofastbuild"],
)
