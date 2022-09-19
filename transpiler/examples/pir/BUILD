load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

fhe_cc_library(
    name = "pir_api_tfhe",
    src = "pir_api.cc",
    hdrs = ["pir_api.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "pir_api_interpreted_tfhe",
    src = "pir_api.cc",
    hdrs = ["pir_api.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

cc_library(
    name = "pir_cloud_service",
    srcs = ["pir_cloud_service.cc"],
    hdrs = ["pir_cloud_service.h"],
    deps = [
        ":pir_api_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/types:span",
        "@tfhe//:libtfhe",
    ],
)

cc_library(
    name = "pir_interpreted_cloud_service",
    srcs = ["pir_cloud_service.cc"],
    hdrs = ["pir_cloud_service.h"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":pir_api_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/types:span",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "pir_client",
    srcs = [
        "pir_api.h",
        "pir_client_main.cc",
    ],
    deps = [
        ":pir_cloud_service",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:span",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "pir_interpreted_client",
    srcs = [
        "pir_api.h",
        "pir_client_main.cc",
    ],
    deps = [
        ":pir_interpreted_cloud_service",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:span",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_test(
    name = "pir_api_tfhe_test",
    size = "large",
    srcs = ["pir_api_tfhe_test.cc"],
    deps = [
        ":pir_api_tfhe",
        "//transpiler/data:tfhe_data",
        "//transpiler/tests:tfhe_test_util",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "pir_api_interpreted_tfhe_test",
    size = "large",
    srcs = ["pir_api_tfhe_test.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":pir_api_interpreted_tfhe",
        # There seems to be a memory leak in TFHE; this fails with heapcheck enabled.
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:tfhe_data",
        "//transpiler/tests:tfhe_test_util",
        "@tfhe//:libtfhe",
    ],
)
