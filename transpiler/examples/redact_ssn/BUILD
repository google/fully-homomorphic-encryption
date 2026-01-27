load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//transpiler:fhe.bzl", "fhe_cc_library")

package(
    default_applicable_licenses = ["@com_google_fully_homomorphic_encryption//:license"],
    default_visibility = ["//visibility:public"],
    features = [
        # Disabling layering check since primitives_tfhe.types.h is built on the fly.
        "-layering_check",
    ],
)

cc_library(
    name = "redact_ssn_plaintext",
    srcs = ["redact_ssn.cc"],
    hdrs = ["redact_ssn.h"],
)

fhe_cc_library(
    name = "redact_ssn_tfhe_yosys_interpreted",
    src = "redact_ssn.cc",
    hdrs = ["redact_ssn.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

cc_binary(
    name = "redact_ssn_tfhe_testbench",
    srcs = ["redact_ssn_tfhe_testbench.cc"],
    deps = [
        ":redact_ssn_tfhe_yosys_interpreted",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/log",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

fhe_cc_library(
    name = "redact_ssn_openfhe_yosys_interpreted",
    src = "redact_ssn.cc",
    hdrs = ["redact_ssn.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

cc_binary(
    name = "redact_ssn_openfhe_testbench",
    srcs = ["redact_ssn_openfhe_testbench.cc"],
    deps = [
        ":redact_ssn_openfhe_yosys_interpreted",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/log",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_test(
    name = "redact_ssn_test",
    srcs = ["redact_ssn_test.cc"],
    deps = [
        ":redact_ssn_plaintext",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

cc_test(
    name = "redact_ssn_tfhe_test",
    size = "large",
    srcs = ["redact_ssn_tfhe_test.cc"],
    tags = ["nosan"],
    deps = [
        ":redact_ssn_plaintext",
        ":redact_ssn_tfhe_yosys_interpreted",
        "@com_google_xls//xls/common/status:matchers",
        # There seems to be a memory leak in TFHE; this fails with heapcheck enabled.
        "@googletest//:gtest_main",
        "@com_google_absl//absl/base",
        "//transpiler/data:tfhe_data",
        "@tfhe//:libtfhe",
    ],
)
