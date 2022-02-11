load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

fhe_cc_library(
    name = "string_cap_tfhe",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    num_opt_passes = 2,
)

fhe_cc_library(
    name = "string_cap_interpreted_tfhe",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    backend = "interpreted_tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_cap_palisade",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    backend = "palisade",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_cap_interpreted_palisade",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    backend = "interpreted_palisade",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_cap_bool",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    backend = "cleartext",
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_binary(
    name = "string_cap_tfhe_testbench",
    srcs = [
        "string_cap.h",
        "string_cap_tfhe_testbench.cc",
    ],
    deps = [
        ":string_cap_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "string_cap_interpreted_tfhe_testbench",
    srcs = [
        "string_cap.h",
        "string_cap_tfhe_testbench.cc",
    ],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":string_cap_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/status",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "string_cap_palisade_testbench",
    srcs = [
        "string_cap.h",
        "string_cap_palisade_testbench.cc",
    ],
    deps = [
        ":string_cap_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "string_cap_interpreted_palisade_testbench",
    srcs = [
        "string_cap.h",
        "string_cap_palisade_testbench.cc",
    ],
    copts = ["-DUSE_INTERPRETED_PALISADE"],
    deps = [
        ":string_cap_interpreted_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)

cc_binary(
    name = "string_cap_bool_testbench",
    srcs = [
        "string_cap.h",
        "string_cap_bool_testbench.cc",
    ],
    deps = [
        ":string_cap_bool",
        "//transpiler/data:boolean_data",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_library(
    name = "string_cap",
    srcs = ["string_cap.cc"],
    hdrs = ["string_cap.h"],
)

cc_test(
    name = "string_cap_test",
    size = "large",
    srcs = ["string_cap_test.cc"],
    deps = [
        ":string_cap",
        ":string_cap_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "string_cap_interpreted_test",
    size = "large",
    srcs = ["string_cap_test.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":string_cap",
        ":string_cap_interpreted_tfhe",
        # There seems to be a memory leak in TFHE; this fails with heapcheck enabled.
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:tfhe_data",
        "@tfhe//:libtfhe",
    ],
)
