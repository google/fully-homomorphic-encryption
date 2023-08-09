load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler:fhe.bzl", "fhe_cc_library")
load("//transpiler/rust:fhe_rs.bzl", "fhe_rust_library")
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")
load("//transpiler/jaxite:fhe_py.bzl", "fhe_py_library")
load("@rules_python//python:defs.bzl", "py_binary")

package(
    default_applicable_licenses = ["@com_google_fully_homomorphic_encryption//:license"],
    default_visibility = ["//visibility:public"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "commons",
    srcs = [
        "commons.cc",
    ],
    hdrs = [
        "commons.h",
    ],
)

cc_library(
    name = "image_processing",
    srcs = [
        "kernel_gaussian_blur.cc",
        "kernel_sharpen.cc",
        "ricker_wavelet.cc",
    ],
    hdrs = [
        "kernel_gaussian_blur.h",
        "kernel_sharpen.h",
        "ricker_wavelet.h",
    ],
    deps = [
        ":commons",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "image_processing_testbench",
    srcs = ["image_processing_testbench.cc"],
    deps = [
        ":commons",
        ":image_processing",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@tfhe//:libtfhe",
    ],
)

fhe_cc_library(
    name = "kernel_gaussian_blur_cleartext",
    src = "kernel_gaussian_blur.cc",
    hdrs = ["kernel_gaussian_blur.h"],
    encryption = "cleartext",
    num_opt_passes = 2,
)

fhe_cc_library(
    name = "kernel_gaussian_blur_yosys_interpreted_cleartext",
    src = "kernel_gaussian_blur.cc",
    hdrs = ["kernel_gaussian_blur.h"],
    encryption = "cleartext",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "kernel_gaussian_blur_tfhe",
    src = "kernel_gaussian_blur.cc",
    hdrs = ["kernel_gaussian_blur.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "kernel_gaussian_blur_interpreted_tfhe",
    src = "kernel_gaussian_blur.cc",
    hdrs = ["kernel_gaussian_blur.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "kernel_gaussian_blur_yosys_interpreted_tfhe",
    src = "kernel_gaussian_blur.cc",
    hdrs = ["kernel_gaussian_blur.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "kernel_gaussian_blur_openfhe",
    src = "kernel_gaussian_blur.cc",
    hdrs = ["kernel_gaussian_blur.h"],
    encryption = "openfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "kernel_gaussian_blur_interpreted_openfhe",
    src = "kernel_gaussian_blur.cc",
    hdrs = ["kernel_gaussian_blur.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "kernel_gaussian_blur_yosys_interpreted_openfhe",
    src = "kernel_gaussian_blur.cc",
    hdrs = ["kernel_gaussian_blur.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "kernel_sharpen_cleartext",
    src = "kernel_sharpen.cc",
    hdrs = ["kernel_sharpen.h"],
    encryption = "cleartext",
)

fhe_cc_library(
    name = "kernel_sharpen_yosys_interpreted_cleartext",
    src = "kernel_sharpen.cc",
    hdrs = ["kernel_sharpen.h"],
    encryption = "cleartext",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "kernel_sharpen_tfhe",
    src = "kernel_sharpen.cc",
    hdrs = ["kernel_sharpen.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "kernel_sharpen_interpreted_tfhe",
    src = "kernel_sharpen.cc",
    hdrs = ["kernel_sharpen.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "kernel_sharpen_yosys_interpreted_tfhe",
    src = "kernel_sharpen.cc",
    hdrs = ["kernel_sharpen.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "kernel_sharpen_openfhe",
    src = "kernel_sharpen.cc",
    hdrs = ["kernel_sharpen.h"],
    encryption = "openfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "kernel_sharpen_interpreted_openfhe",
    src = "kernel_sharpen.cc",
    hdrs = ["kernel_sharpen.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "kernel_sharpen_yosys_interpreted_openfhe",
    src = "kernel_sharpen.cc",
    hdrs = ["kernel_sharpen.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "ricker_wavelet_cleartext",
    src = "ricker_wavelet.cc",
    hdrs = ["ricker_wavelet.h"],
    encryption = "cleartext",
    num_opt_passes = 2,
)

fhe_cc_library(
    name = "ricker_wavelet_yosys_interpreted_cleartext",
    src = "ricker_wavelet.cc",
    hdrs = ["ricker_wavelet.h"],
    encryption = "cleartext",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "ricker_wavelet_tfhe",
    src = "ricker_wavelet.cc",
    hdrs = ["ricker_wavelet.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "ricker_wavelet_interpreted_tfhe",
    src = "ricker_wavelet.cc",
    hdrs = ["ricker_wavelet.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "ricker_wavelet_yosys_interpreted_tfhe",
    src = "ricker_wavelet.cc",
    hdrs = ["ricker_wavelet.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "ricker_wavelet_openfhe",
    src = "ricker_wavelet.cc",
    hdrs = ["ricker_wavelet.h"],
    encryption = "openfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "ricker_wavelet_interpreted_openfhe",
    src = "ricker_wavelet.cc",
    hdrs = ["ricker_wavelet.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "ricker_wavelet_yosys_interpreted_openfhe",
    src = "ricker_wavelet.cc",
    hdrs = ["ricker_wavelet.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

cc_binary(
    name = "image_processing_cleartext_testbench",
    srcs = ["image_processing_cleartext_testbench.cc"],
    deps = [
        ":commons",
        ":kernel_gaussian_blur_cleartext",
        ":kernel_sharpen_cleartext",
        ":ricker_wavelet_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_test(
    name = "image_processing_yosys_interpreted_cleartext_test",
    srcs = ["image_processing_cleartext_test.cc"],
    deps = [
        ":commons",
        ":image_processing",
        ":kernel_gaussian_blur_yosys_interpreted_cleartext",
        ":kernel_sharpen_yosys_interpreted_cleartext",
        ":ricker_wavelet_yosys_interpreted_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_test(
    name = "ricker_wavelet_test",
    srcs = ["ricker_wavelet_test.cc"],
    deps = [
        ":image_processing",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

cc_binary(
    name = "image_processing_yosys_interpreted_cleartext_testbench",
    srcs = ["image_processing_cleartext_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_CLEARTEXT"],
    deps = [
        ":commons",
        ":kernel_gaussian_blur_yosys_interpreted_cleartext",
        ":kernel_sharpen_yosys_interpreted_cleartext",
        ":ricker_wavelet_yosys_interpreted_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "image_processing_tfhe_testbench",
    srcs = ["image_processing_tfhe_testbench.cc"],
    deps = [
        ":commons",
        ":kernel_gaussian_blur_tfhe",
        ":kernel_sharpen_tfhe",
        ":ricker_wavelet_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "image_processing_interpreted_tfhe_testbench",
    srcs = ["image_processing_tfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_TFHE"],
    deps = [
        ":commons",
        ":kernel_gaussian_blur_interpreted_tfhe",
        ":kernel_sharpen_interpreted_tfhe",
        ":ricker_wavelet_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "image_processing_yosys_interpreted_tfhe_testbench",
    srcs = ["image_processing_tfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":commons",
        ":kernel_gaussian_blur_yosys_interpreted_tfhe",
        ":kernel_sharpen_yosys_interpreted_tfhe",
        ":ricker_wavelet_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "image_processing_openfhe_testbench",
    srcs = ["image_processing_openfhe_testbench.cc"],
    deps = [
        ":commons",
        ":kernel_gaussian_blur_openfhe",
        ":kernel_sharpen_openfhe",
        ":ricker_wavelet_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "image_processing_interpreted_openfhe_testbench",
    srcs = ["image_processing_openfhe_testbench.cc"],
    copts = ["-DUSE_INTERPRETED_OPENFHE"],
    deps = [
        ":commons",
        ":kernel_gaussian_blur_interpreted_openfhe",
        ":kernel_sharpen_interpreted_openfhe",
        ":ricker_wavelet_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "image_processing_yosys_interpreted_openfhe_testbench",
    srcs = ["image_processing_openfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_OPENFHE"],
    deps = [
        ":commons",
        ":kernel_gaussian_blur_yosys_interpreted_openfhe",
        ":kernel_sharpen_yosys_interpreted_openfhe",
        ":ricker_wavelet_yosys_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

fhe_rust_library(
    name = "ricker_wavelet_rs_fhe_lib",
    src = "ricker_wavelet.cc",
    hdrs = ["ricker_wavelet.h"],
)

rust_library(
    name = "ricker_wavelet_rs_lib",
    srcs = ["ricker_wavelet_rs_lib.rs"],
    deps = [
        ":ricker_wavelet_rs_fhe_lib",
        "@crate_index//:tfhe",
    ],
)

rust_test(
    name = "ricker_wavelet_rs_test",
    size = "large",
    crate = ":ricker_wavelet_rs_lib",
    tags = ["nofastbuild"],
)

fhe_py_library(
    name = "kernel_gaussian_blur_fhe_py_lib",
    src = "kernel_gaussian_blur.cc",
    hdrs = ["kernel_gaussian_blur.h"],
    lut_size = 3,
    num_opt_passes = 2,
    optimizer = "yosys",
)

py_binary(
    name = "kernel_gaussian_blur_jaxite_main",
    srcs = ["kernel_gaussian_blur.py"],
    main = "kernel_gaussian_blur.py",
    deps = [
        ":kernel_gaussian_blur_fhe_py_lib",
        "@com_google_absl_py//absl/testing:absltest",
        "@transpiler_pip_deps//pypi__jaxite",
    ],
)

exports_files([
    "kernel_gaussian_blur.cc",
    "kernel_gaussian_blur.h",
])
