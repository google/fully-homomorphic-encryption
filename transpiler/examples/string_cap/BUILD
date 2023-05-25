load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load(
    "//transpiler:fhe_demo.bzl",
    "gen_demos",
    "gen_fhe_cc_library_for_all_optimizers_translators_cryptosystems",
)

package(
    default_applicable_licenses = ["@com_google_fully_homomorphic_encryption//:license"],
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

gen_fhe_cc_library_for_all_optimizers_translators_cryptosystems(
    name = "string_cap",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    num_opt_passes = 2,
)

gen_demos(
    name = "string_cap_tfhe",
    srcs = [
        "string_cap.h",
        "string_cap_tfhe_testbench.cc",
    ],
    encryption = "tfhe",
    deps = [
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

gen_demos(
    name = "string_cap_openfhe",
    srcs = [
        "string_cap.h",
        "string_cap_openfhe_testbench.cc",
    ],
    encryption = "openfhe",
    deps = [
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

gen_demos(
    name = "string_cap_cleartext",
    srcs = [
        "string_cap.h",
        "string_cap_cleartext_testbench.cc",
    ],
    encryption = "cleartext",
    deps = [
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_library(
    name = "string_cap_plaintext",
    srcs = ["string_cap.cc"],
    hdrs = ["string_cap.h"],
)

cc_test(
    name = "string_cap_tfhe_xls_transpiled_test",
    size = "large",
    srcs = ["string_cap_test.cc"],
    deps = [
        ":string_cap_plaintext",
        ":string_cap_tfhe_xls_transpiled",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "string_cap_tfhe_xls_interpreted_test",
    size = "large",
    srcs = ["string_cap_test.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":string_cap_plaintext",
        ":string_cap_tfhe_xls_interpreted",
        # There seems to be a memory leak in TFHE; this fails with heapcheck enabled.
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:tfhe_data",
        "@tfhe//:libtfhe",
    ],
)

exports_files([
    "string_cap.cc",
    "string_cap.h",
])
