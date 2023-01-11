load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load(
    "//transpiler:fhe_demo.bzl",
    "gen_demos",
    "gen_fhe_cc_library_for_all_optimizers_translators_cryptosystems",
)

licenses(["notice"])

gen_fhe_cc_library_for_all_optimizers_translators_cryptosystems(
    name = "string_cap_char",
    src = "string_cap_char.cc",
    hdrs = ["string_cap_char.h"],
    num_opt_passes = 2,
)

gen_demos(
    name = "string_cap_char_tfhe",
    srcs = [
        "string_cap_char.h",
        "string_cap_char_tfhe_testbench.cc",
    ],
    encryption = "tfhe",
    deps = [
        ":string_cap_char_plaintext",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

gen_demos(
    name = "string_cap_char_openfhe",
    srcs = [
        "string_cap_char.h",
        "string_cap_char_openfhe_testbench.cc",
    ],
    encryption = "openfhe",
    deps = [
        "string_cap_char_plaintext",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

gen_demos(
    name = "string_cap_char_cleartext",
    srcs = [
        "string_cap_char.h",
        "string_cap_char_cleartext_testbench.cc",
    ],
    encryption = "cleartext",
    deps = [
        "string_cap_char_plaintext",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_library(
    name = "string_cap_char_plaintext",
    srcs = ["string_cap_char.cc"],
    hdrs = ["string_cap_char.h"],
)

cc_test(
    name = "string_cap_char_tfhe_xls_transpiled_test",
    size = "large",
    srcs = ["string_cap_char_test.cc"],
    deps = [
        ":string_cap_char_plaintext",
        ":string_cap_char_tfhe_xls_transpiled",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "string_cap_char_tfhe_xls_interpreted_test",
    size = "large",
    srcs = ["string_cap_char_test.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":string_cap_char_plaintext",
        ":string_cap_char_tfhe_xls_interpreted",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@tfhe//:libtfhe",
    ],
)

exports_files([
    "string_cap_char.cc",
    "string_cap_char.h",
])
