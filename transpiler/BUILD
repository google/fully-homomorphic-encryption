load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

bzl_library(
    name = "fhe_bzl",
    srcs = ["fhe.bzl"],
)

cc_library(
    name = "abstract_xls_transpiler",
    hdrs = ["abstract_xls_transpiler.h"],
    deps = [
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
    name = "tfhe_transpiler",
    srcs = ["tfhe_transpiler.cc"],
    hdrs = ["tfhe_transpiler.h"],
    deps = [
        ":abstract_xls_transpiler",
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
    name = "interpreted_tfhe_transpiler",
    srcs = ["interpreted_tfhe_transpiler.cc"],
    hdrs = ["interpreted_tfhe_transpiler.h"],
    deps = [
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
    name = "tfhe_runner",
    srcs = ["tfhe_runner.cc"],
    hdrs = ["tfhe_runner.h"],
    deps = [
        ":abstract_xls_transpiler",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_xls//xls/common/file:filesystem",
        "@com_google_xls//xls/common/status:status_macros",
        "@com_google_xls//xls/contrib/xlscc:metadata_output_cc_proto",
        "@com_google_xls//xls/ir",
        "@com_google_xls//xls/ir:ir_parser",
        "@com_google_xls//xls/ir:type",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "tfhe_runner_test",
    srcs = ["tfhe_runner_test.cc"],
    deps = [
        ":tfhe_runner",
        "//transpiler/data:fhe_data",
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
    name = "cc_transpiler",
    srcs = ["cc_transpiler.cc"],
    hdrs = ["cc_transpiler.h"],
    deps = [
        ":abstract_xls_transpiler",
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

cc_binary(
    name = "transpiler",
    srcs = ["transpiler_main.cc"],
    data = [
        "@com_google_xls//xls/tools:booleanify_main",
        "@com_google_xls//xls/tools:opt_main",
    ],
    deps = [
        ":cc_transpiler",
        ":interpreted_tfhe_transpiler",
        ":tfhe_transpiler",
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
        "@com_google_xls//xls/ir:type",
        "@com_google_xls//xls/public:function_builder",
    ],
)

exports_files(glob(["*.liberty"]))
