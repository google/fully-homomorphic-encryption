# The codegen steps for the jaxite FHE library target

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(
    default_applicable_licenses = ["@com_google_fully_homomorphic_encryption//:license"],
    default_visibility = ["//visibility:public"],
)

bzl_library(
    name = "fhe_py_bzl",
    srcs = ["fhe_py.bzl"],
    deps = [
        # copybara: pytype_bzl_dep,
        "//transpiler:fhe_common_bzl",
        "//transpiler:fhe_xls_bzl",
        "//transpiler:fhe_yosys_bzl",
        "//transpiler:parsers_bzl",
    ],
)

cc_library(
    name = "testing",
    testonly = True,
    srcs = ["testing.cc"],
    hdrs = ["testing.h"],
    deps = [
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
    ],
)

cc_library(
    name = "jaxite_xls_transpiler",
    srcs = ["jaxite_xls_transpiler.cc"],
    hdrs = ["jaxite_xls_transpiler.h"],
    deps = [
        "//transpiler:abstract_xls_transpiler",
        "//transpiler/util:string",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/public:ir",
    ],
)

cc_test(
    name = "jaxite_xls_transpiler_test",
    srcs = ["jaxite_xls_transpiler_test.cc"],
    deps = [
        ":jaxite_xls_transpiler",
        ":testing",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/public:function_builder",
        "@com_google_xls//xls/public:ir",
        "@com_google_xls//xls/public:ir_parser",
    ],
)

cc_library(
    name = "yosys_transpiler",
    srcs = ["yosys_transpiler.cc"],
    hdrs = ["yosys_transpiler.h"],
    deps = [
        "//transpiler:netlist_utils",
        "//transpiler/util:string",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/status:status_macros",
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
        ":yosys_transpiler",
        "@com_google_absl//absl/strings",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

cc_binary(
    name = "transpiler",
    srcs = ["transpiler_main.cc"],
    data = [
        "@com_google_xls//xls/dev_tools:booleanify_main",
        "@com_google_xls//xls/tools:opt_main",
    ],
    deps = [
        ":jaxite_xls_transpiler",
        ":yosys_transpiler",
        "//transpiler:pipeline_enums",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/status",
        "@com_google_xls//xls/common/file:filesystem",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/public:ir",
        "@com_google_xls//xls/public:ir_parser",
    ],
)
