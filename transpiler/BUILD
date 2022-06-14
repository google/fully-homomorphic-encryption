load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

bzl_library(
    name = "fhe_bzl",
    srcs = ["fhe.bzl"],
    deps = ["//transpiler/data:primitives_bzl"],
)

cc_library(
    name = "abstract_xls_transpiler",
    hdrs = ["abstract_xls_transpiler.h"],
    deps = [
        ":common_transpiler",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/ir",
    ],
)

cc_library(
    name = "common_runner",
    srcs = ["common_runner.cc"],
    hdrs = ["common_runner.h"],
    deps = [
        "@com_google_xls//xls/common/logging",
    ],
)

cc_library(
    name = "common_transpiler",
    srcs = ["common_transpiler.cc"],
    hdrs = ["common_transpiler.h"],
    deps = [
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:optional",
        "@com_google_absl//absl/types:span",
        "@com_google_xls//xls/common/logging",
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
        "@com_google_xls//xls/ir",
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
        "@com_google_xls//xls/ir",
        "@com_google_xls//xls/public:value",
    ],
)

cc_library(
    name = "yosys_transpiler",
    srcs = ["yosys_transpiler.cc"],
    hdrs = ["yosys_transpiler.h"],
    deps = [
        ":common_transpiler",
        ":pipeline_enums",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf",
        "@com_google_xls//xls/common/logging",
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
        "@com_google_xls//xls/ir:bits",
        "@com_google_xls//xls/ir:bits_ops",
        "@com_google_xls//xls/netlist:cell_library",
        "@com_google_xls//xls/netlist:function_extractor",
        "@com_google_xls//xls/netlist:interpreter",
        "@com_google_xls//xls/netlist:lib_parser",
        "@com_google_xls//xls/netlist:netlist_parser",
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
        "@com_google_xls//xls/ir:bits",
        "@com_google_xls//xls/ir:bits_ops",
        "@com_google_xls//xls/netlist",
        "@com_google_xls//xls/netlist:cell_library",
        "@com_google_xls//xls/netlist:function_extractor",
        "@com_google_xls//xls/netlist:interpreter",
        "@com_google_xls//xls/netlist:lib_parser",
        "@com_google_xls//xls/netlist:netlist_parser",
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
        "@com_google_xls//xls/ir:bits",
        "@com_google_xls//xls/ir:bits_ops",
        "@com_google_xls//xls/netlist",
        "@com_google_xls//xls/netlist:cell_library",
        "@com_google_xls//xls/netlist:function_extractor",
        "@com_google_xls//xls/netlist:interpreter",
        "@com_google_xls//xls/netlist:lib_parser",
        "@com_google_xls//xls/netlist:netlist_parser",
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
        "@com_google_xls//xls/ir:ir_parser",
    ],
)

cc_library(
    name = "abstract_xls_runner",
    hdrs = ["abstract_xls_runner.h"],
    deps = [
        ":abstract_xls_transpiler",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
        "@com_google_xls//xls/common/file:filesystem",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/ir",
        "@com_google_xls//xls/ir:ir_parser",
        "@com_google_xls//xls/ir:type",
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
    name = "cc_transpiler",
    srcs = ["cc_transpiler.cc"],
    hdrs = ["cc_transpiler.h"],
    deps = [
        ":abstract_xls_transpiler",
        ":common_transpiler",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:span",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/ir",
        "@com_google_xls//xls/public:value",
    ],
)

cc_test(
    name = "cc_transpiler_test",
    srcs = ["cc_transpiler_test.cc"],
    deps = [
        ":cc_transpiler",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/ir",
        "@com_google_xls//xls/ir:type",
        "@com_google_xls//xls/public:function_builder",
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
        "@com_google_xls//xls/ir",
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
        "@com_google_xls//xls/ir",
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
        "@com_google_xls//xls/tools:booleanify_main",
        "@com_google_xls//xls/tools:opt_main",
    ],
    deps = [
        ":cc_transpiler",
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
        "@com_google_xls//xls/ir",
        "@com_google_xls//xls/ir:bits",
        "@com_google_xls//xls/ir:ir_parser",
    ],
)

cc_test(
    name = "tfhe_transpiler_test",
    srcs = ["tfhe_transpiler_test.cc"],
    data = [
        "@com_google_xls//xls/tools:booleanify_main",
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
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/file:filesystem",
        "@com_google_xls//xls/common/status:matchers",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/ir",
        "@com_google_xls//xls/ir:ir_parser",
        "@com_google_xls//xls/ir:source_location",
        "@com_google_xls//xls/ir:type",
        "@com_google_xls//xls/public:function_builder",
    ],
)

exports_files(glob(["*.liberty"]))
