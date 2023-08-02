# The codegen steps for the tfhe-rs library target.

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(
    default_applicable_licenses = ["@com_google_fully_homomorphic_encryption//:license"],
    default_visibility = ["//visibility:public"],
)

bzl_library(
    name = "fhe_rs_bzl",
    srcs = ["fhe_rs.bzl"],
    deps = [
        "//transpiler:fhe_common_bzl",
        "//transpiler:fhe_xls_bzl",
        "//transpiler:fhe_yosys_bzl",
        "//transpiler:parsers_bzl",
        "@rules_rust//rust:bzl_lib",
    ],
)

cc_library(
    name = "tfhe_rs_templates",
    srcs = [],
    hdrs = ["tfhe_rs_templates.h"],
    deps = [
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "yosys_transpiler",
    srcs = ["yosys_transpiler.cc"],
    hdrs = ["yosys_transpiler.h"],
    deps = [
        ":tfhe_rs_templates",
        "//transpiler:netlist_utils",
        "//transpiler/util:string",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/netlist",
        "@com_google_xls//xls/netlist:cell_library",
        "@com_google_xls//xls/netlist:function_extractor",
        "@com_google_xls//xls/netlist:lib_parser",
        "@com_google_xls//xls/netlist:netlist_parser",
    ],
)

cc_test(
    name = "yosys_transpiler_test",
    srcs = ["yosys_transpiler_test.cc"],
    deps = [
        ":tfhe_rs_templates",
        ":yosys_transpiler",
        "//transpiler:netlist_utils",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/netlist",
        "@com_google_xls//xls/netlist:cell_library",
    ],
)

cc_binary(
    name = "transpiler",
    srcs = ["transpiler_main.cc"],
    deps = [
        ":yosys_transpiler",
        "//transpiler:metadata_utils",
        "//transpiler:netlist_utils",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/status",
        "@com_google_xls//xls/common/file:filesystem",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/netlist:cell_library",
    ],
)
