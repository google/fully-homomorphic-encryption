load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

fhe_cc_library(
    name = "string_cap_tfhe",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    encryption = "tfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_cap_interpreted_tfhe",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    encryption = "tfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_cap_yosys_interpreted_tfhe",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    encryption = "tfhe",
    interpreter = True,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "string_cap_openfhe",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    encryption = "openfhe",
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_cap_interpreted_openfhe",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "xls",
)

fhe_cc_library(
    name = "string_cap_yosys_interpreted_openfhe",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    encryption = "openfhe",
    interpreter = True,
    num_opt_passes = 2,
    optimizer = "yosys",
)

fhe_cc_library(
    name = "string_cap_cleartext",
    src = "string_cap.cc",
    hdrs = ["string_cap.h"],
    encryption = "cleartext",
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
    name = "string_cap_yosys_interpreted_tfhe_testbench",
    srcs = [
        "string_cap.h",
        "string_cap_tfhe_testbench.cc",
    ],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":string_cap_yosys_interpreted_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/status",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "string_cap_openfhe_testbench",
    srcs = [
        "string_cap.h",
        "string_cap_openfhe_testbench.cc",
    ],
    deps = [
        ":string_cap_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "string_cap_interpreted_openfhe_testbench",
    srcs = [
        "string_cap.h",
        "string_cap_openfhe_testbench.cc",
    ],
    copts = ["-DUSE_INTERPRETED_OPENFHE"],
    deps = [
        ":string_cap_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "string_cap_yosys_interpreted_openfhe_testbench",
    srcs = [
        "string_cap.h",
        "string_cap_openfhe_testbench.cc",
    ],
    copts = ["-DUSE_YOSYS_INTERPRETED_OPENFHE"],
    deps = [
        ":string_cap_yosys_interpreted_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

cc_binary(
    name = "string_cap_cleartext_testbench",
    srcs = [
        "string_cap.h",
        "string_cap_cleartext_testbench.cc",
    ],
    deps = [
        ":string_cap_cleartext",
        "//transpiler/data:cleartext_data",
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
