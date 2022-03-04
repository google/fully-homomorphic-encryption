load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

fhe_cc_library(
    name = "ifte_yosys_cleartext",
    src = "ifte.cc",
    hdrs = ["ifte.h"],
    encryption = "cleartext",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "ifte_yosys_interpreted_tfhe",
    src = "ifte.cc",
    hdrs = ["ifte.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "ifte_yosys_interpreted_palisade",
    src = "ifte.cc",
    hdrs = ["ifte.h"],
    encryption = "palisade",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "ifte_cleartext",
    src = "ifte.cc",
    hdrs = ["ifte.h"],
    encryption = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "ifte_tfhe",
    src = "ifte.cc",
    hdrs = ["ifte.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

cc_binary(
    name = "ifte_cleartext_testbench",
    srcs = ["ifte_cleartext_testbench.cc"],
    deps = [
        ":ifte_cleartext",
        "//transpiler/data:boolean_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "ifte_yosys_cleartext_testbench",
    srcs = ["ifte_cleartext_testbench.cc"],
    copts = ["-DUSE_YOSYS_CLEARTEXT"],
    deps = [
        ":ifte_yosys_cleartext",
        "//transpiler/data:boolean_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "ifte_tfhe_testbench",
    srcs = ["ifte_tfhe_testbench.cc"],
    deps = [
        ":ifte_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "ifte_yosys_interpreted_tfhe_testbench",
    srcs = ["ifte_tfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":ifte_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "ifte_yosys_interpreted_palisade_testbench",
    srcs = ["ifte_palisade_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_PALISADE"],
    deps = [
        ":ifte_yosys_interpreted_palisade",
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@palisade//:binfhe",
    ],
)
