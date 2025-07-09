load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(
    default_applicable_licenses = ["@com_google_fully_homomorphic_encryption//:license"],
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

bzl_library(
    name = "fhe_common_bzl",
    srcs = ["fhe_common.bzl"],
)

bzl_library(
    name = "parsers_bzl",
    srcs = ["parsers.bzl"],
    deps = [
        ":fhe_common_bzl",
    ],
)

bzl_library(
    name = "fhe_xls_bzl",
    srcs = ["fhe_xls.bzl"],
    deps = [
        ":fhe_common_bzl",
        ":parsers_bzl",
    ],
)

bzl_library(
    name = "fhe_yosys_bzl",
    srcs = ["fhe_yosys.bzl"],
    deps = [
        ":fhe_common_bzl",
    ],
)

bzl_library(
    name = "fhe_structs_bzl",
    srcs = ["fhe_structs.bzl"],
    deps = [
        ":fhe_common_bzl",
        ":parsers_bzl",
    ],
)

bzl_library(
    name = "fhe_bzl",
    srcs = ["fhe.bzl"],
    deps = [
        ":fhe_common_bzl",
        ":fhe_structs_bzl",
        ":fhe_xls_bzl",
        ":fhe_yosys_bzl",
        ":parsers_bzl",
        "//transpiler/data:primitives_bzl",
    ],
)

bzl_library(
    name = "fhe_demo_bzl",
    srcs = ["fhe_demo.bzl"],
    deps = [
        ":fhe_bzl",
    ],
)

cc_library(
    name = "abstract_xls_transpiler",
    hdrs = ["abstract_xls_transpiler.h"],
    deps = [
        ":common_transpiler",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/protected:ir",
        "@com_google_xls//xls/public:ir",
    ],
)

cc_library(
    name = "common_runner",
    srcs = ["common_runner.cc"],
    hdrs = ["common_runner.h"],
    deps = [
        "@com_google_absl//absl/log:check",
    ],
)

cc_library(
    name = "common_transpiler",
    srcs = ["common_transpiler.cc"],
    hdrs = ["common_transpiler.h"],
    deps = [
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:optional",
        "@com_google_absl//absl/types:span",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
    ],
)

cc_library(
    name = "tfhe_transpiler",
    srcs = ["tfhe_transpiler.cc"],
    hdrs = ["tfhe_transpiler.h"],
    deps = [
        ":abstract_xls_transpiler",
        ":common_transpiler",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/public:ir",
        "@com_google_xls//xls/public:value",
    ],
)

cc_library(
    name = "interpreted_tfhe_transpiler",
    srcs = ["interpreted_tfhe_transpiler.cc"],
    hdrs = ["interpreted_tfhe_transpiler.h"],
    deps = [
        ":common_transpiler",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/public:ir",
        "@com_google_xls//xls/public:value",
    ],
)

cc_library(
    name = "metadata_utils",
    srcs = ["metadata_utils.cc"],
    hdrs = ["metadata_utils.h"],
    deps = [
        ":netlist_utils",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/netlist",
        "@com_google_xls//xls/netlist:cell_library",
        "@com_google_xls//xls/netlist:function_extractor",
        "@com_google_xls//xls/netlist:lib_parser",
        "@com_google_xls//xls/netlist:netlist_parser",
        "@com_nlohmann_json//:json",
    ],
)

cc_test(
    name = "metadata_utils_test",
    srcs = ["metadata_utils_test.cc"],
    deps = [
        ":metadata_utils",
        ":netlist_utils",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/netlist",
        "@com_google_xls//xls/netlist:cell_library",
        "@com_google_xls//xls/netlist:function_extractor",
        "@com_google_xls//xls/netlist:lib_parser",
        "@com_google_xls//xls/netlist:netlist_parser",
    ],
)

cc_library(
    name = "netlist_utils",
    srcs = ["netlist_utils.cc"],
    hdrs = ["netlist_utils.h"],
    deps = [
        ":graph",
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
    name = "netlist_utils_test",
    srcs = ["netlist_utils_test.cc"],
    deps = [
        ":netlist_utils",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/netlist",
        "@com_google_xls//xls/netlist:cell_library",
        "@com_google_xls//xls/netlist:function_extractor",
        "@com_google_xls//xls/netlist:lib_parser",
        "@com_google_xls//xls/netlist:netlist_parser",
    ],
)

cc_library(
    name = "yosys_transpiler",
    srcs = ["yosys_transpiler.cc"],
    hdrs = ["yosys_transpiler.h"],
    deps = [
        ":common_transpiler",
        ":pipeline_enums",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
    ],
)

cc_library(
    name = "yosys_cleartext_runner",
    srcs = ["yosys_cleartext_runner.cc"],
    hdrs = ["yosys_cleartext_runner.h"],
    deps = [
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@com_google_protobuf//:protobuf",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/netlist:cell_library",
        "@com_google_xls//xls/netlist:function_extractor",
        "@com_google_xls//xls/netlist:interpreter",
        "@com_google_xls//xls/netlist:lib_parser",
        "@com_google_xls//xls/netlist:netlist_parser",
        "@com_google_xls//xls/public:value",
    ],
)

cc_library(
    name = "yosys_tfhe_runner",
    srcs = ["yosys_tfhe_runner.cc"],
    hdrs = ["yosys_tfhe_runner.h"],
    deps = [
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@com_google_protobuf//:protobuf",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/netlist",
        "@com_google_xls//xls/netlist:cell_library",
        "@com_google_xls//xls/netlist:function_extractor",
        "@com_google_xls//xls/netlist:interpreter",
        "@com_google_xls//xls/netlist:lib_parser",
        "@com_google_xls//xls/netlist:netlist_parser",
        "@com_google_xls//xls/public:value",
    ],
)

cc_library(
    name = "yosys_openfhe_runner",
    srcs = ["yosys_openfhe_runner.cc"],
    hdrs = ["yosys_openfhe_runner.h"],
    deps = [
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@com_google_protobuf//:protobuf",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/protected:netlist",
        "@com_google_xls//xls/public:value",
        "@openfhe//:binfhe",
    ],
)

cc_test(
    name = "yosys_cleartext_runner_test",
    srcs = ["yosys_cleartext_runner_test.cc"],
    deps = [
        ":yosys_cleartext_runner",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
    ],
)

cc_library(
    name = "tfhe_runner",
    srcs = ["tfhe_runner.cc"],
    hdrs = ["tfhe_runner.h"],
    deps = [
        ":abstract_xls_runner",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/types:optional",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "tfhe_runner_test",
    srcs = ["tfhe_runner_test.cc"],
    deps = [
        ":tfhe_runner",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/public:ir_parser",
    ],
)

cc_library(
    name = "abstract_xls_runner",
    hdrs = ["abstract_xls_runner.h"],
    deps = [
        ":abstract_xls_transpiler",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
        "@com_google_xls//xls/common/file:filesystem",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/public:ir",
        "@com_google_xls//xls/public:ir_parser",
    ],
)

cc_library(
    name = "openfhe_runner",
    srcs = ["openfhe_runner.cc"],
    hdrs = ["openfhe_runner.h"],
    deps = [
        ":abstract_xls_runner",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/types:optional",
        "@openfhe//:binfhe",
    ],
)

cc_library(
    name = "cleartext_transpiler",
    srcs = ["cleartext_transpiler.cc"],
    hdrs = ["cleartext_transpiler.h"],
    deps = [
        ":abstract_xls_transpiler",
        ":common_transpiler",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:span",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/public:ir",
        "@com_google_xls//xls/public:value",
    ],
)

cc_test(
    name = "cleartext_transpiler_test",
    srcs = ["cleartext_transpiler_test.cc"],
    deps = [
        ":cleartext_transpiler",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/public:function_builder",
        "@com_google_xls//xls/public:ir",
    ],
)

cc_library(
    name = "openfhe_transpiler",
    srcs = ["openfhe_transpiler.cc"],
    hdrs = ["openfhe_transpiler.h"],
    deps = [
        ":abstract_xls_transpiler",
        ":common_transpiler",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/public:ir",
        "@com_google_xls//xls/public:value",
    ],
)

cc_library(
    name = "interpreted_openfhe_transpiler",
    srcs = ["interpreted_openfhe_transpiler.cc"],
    hdrs = ["interpreted_openfhe_transpiler.h"],
    deps = [
        ":common_transpiler",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/public:ir",
        "@com_google_xls//xls/public:value",
    ],
)

cc_library(
    name = "pipeline_enums",
    hdrs = ["pipeline_enums.h"],
    deps = ["@com_google_absl//absl/strings"],
)

cc_binary(
    name = "transpiler",
    srcs = ["transpiler_main.cc"],
    data = [
        "@com_google_xls//xls/dev_tools:booleanify_main",
        "@com_google_xls//xls/tools:opt_main",
    ],
    deps = [
        ":cleartext_transpiler",
        ":interpreted_openfhe_transpiler",
        ":interpreted_tfhe_transpiler",
        ":openfhe_transpiler",
        ":pipeline_enums",
        ":tfhe_transpiler",
        ":yosys_transpiler",
        "//transpiler/util:subprocess",
        "//transpiler/util:temp_file",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/file:filesystem",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/public:ir",
        "@com_google_xls//xls/public:ir_parser",
        "@com_google_xls//xls/public:value",
    ],
)

cc_test(
    name = "tfhe_transpiler_test",
    srcs = ["tfhe_transpiler_test.cc"],
    data = [
        "@com_google_xls//xls/dev_tools:booleanify_main",
    ],
    deps = [
        ":tfhe_transpiler",
        "//transpiler/util:runfiles",
        "//transpiler/util:subprocess",
        "//transpiler/util:temp_file",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_absl//absl/types:span",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/file:filesystem",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/public:function_builder",
        "@com_google_xls//xls/public:ir",
        "@com_google_xls//xls/public:ir_parser",
    ],
)

cc_library(
    name = "graph",
    hdrs = ["graph.h"],
    deps = [
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/types:optional",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_test(
    name = "graph_test",
    srcs = ["graph_test.cc"],
    deps = [
        ":graph",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)
