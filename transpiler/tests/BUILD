load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//transpiler:fhe.bzl", "fhe_cc_library")

licenses(["notice"])

_PRIMITIVE_TYPES = [
    "array",
    "char",
    "int",
    "long",
]

[
    [
        fhe_cc_library(
            name = "%s_tfhe" % i,
            src = "%s.cc" % i,
            hdrs = [],
            encryption = "tfhe",
            num_opt_passes = 2,
            optimizer = "xls",
        ),
        fhe_cc_library(
            name = "%s_openfhe" % i,
            src = "%s.cc" % i,
            hdrs = [],
            encryption = "openfhe",
            num_opt_passes = 2,
            optimizer = "xls",
        ),
    ]
    for i in _PRIMITIVE_TYPES
]

fhe_cc_library(
    name = "namespaced_struct_tfhe",
    src = "namespaced_struct.cc",
    hdrs = ["namespaced_struct.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "namespaced_struct_openfhe",
    src = "namespaced_struct.cc",
    hdrs = ["namespaced_struct.h"],
    encryption = "openfhe",
    optimizer = "xls",
)

cc_library(
    name = "tfhe_test_util",
    testonly = True,
    srcs = ["tfhe_test_util.cc"],
    hdrs = ["tfhe_test_util.h"],
    visibility = [
        "//:__subpackages__",
    ],
    deps = [
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

cc_library(
    name = "openfhe_test_util",
    testonly = True,
    srcs = ["openfhe_test_util.cc"],
    hdrs = ["openfhe_test_util.h"],
    visibility = [
        "//:__subpackages__",
    ],
    deps = [
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest",
        "@com_google_xls//xls/common/status:matchers",
        "@openfhe//:binfhe",
    ],
)

cc_test(
    name = "types_test",
    srcs = ["types_test.cc"],
    shard_count = 4,
    deps = [":%s_tfhe" % i for i in _PRIMITIVE_TYPES] + [
        ":namespaced_struct_tfhe",
        ":struct2_tfhe",
        ":struct_tfhe",
        ":tfhe_test_util",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "openfhe_types_test",
    srcs = ["openfhe_types_test.cc"],
    shard_count = 4,
    deps = [":%s_openfhe" % i for i in _PRIMITIVE_TYPES] + [
        ":namespaced_struct_openfhe",
        ":openfhe_test_util",
        ":struct2_openfhe",
        ":struct_openfhe",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@openfhe//:binfhe",
    ],
)

_CONTROL_STRUCTURE = [
    "if",
    "for",
    "function",
    "nested_for",
    "switch",
]

[
    [
        fhe_cc_library(
            name = "%s_tfhe" % i,
            src = "%s.cc" % i,
            hdrs = [],
            encryption = "tfhe",
            num_opt_passes = 2,
            optimizer = "xls",
        ),
        fhe_cc_library(
            name = "%s_openfhe" % i,
            src = "%s.cc" % i,
            hdrs = [],
            encryption = "openfhe",
            num_opt_passes = 2,
            optimizer = "xls",
        ),
    ]
    for i in _CONTROL_STRUCTURE
]

cc_test(
    name = "control_structure_test",
    size = "medium",
    srcs = ["control_structure_test.cc"],
    shard_count = 8,
    deps = [":%s_tfhe" % i for i in _CONTROL_STRUCTURE] + [
        ":tfhe_test_util",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "openfhe_control_structure_test",
    size = "medium",
    srcs = ["openfhe_control_structure_test.cc"],
    shard_count = 8,
    deps = [":%s_openfhe" % i for i in _CONTROL_STRUCTURE] + [
        ":openfhe_test_util",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
        "@openfhe//:binfhe",
    ],
)

fhe_cc_library(
    name = "function_cleartext",
    src = "function.cc",
    hdrs = [],
    encryption = "cleartext",
    num_opt_passes = 1,
    optimizer = "xls",
)

fhe_cc_library(
    name = "out_params_cleartext",
    src = "out_params.cc",
    hdrs = [],
    encryption = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "out_params_with_return_cleartext",
    src = "out_params_with_return.cc",
    hdrs = [],
    encryption = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "single_out_param_cleartext",
    src = "single_out_param.cc",
    hdrs = [],
    encryption = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "single_out_param_with_return_cleartext",
    src = "single_out_param_with_return.cc",
    hdrs = [],
    encryption = "cleartext",
    optimizer = "xls",
)

cc_test(
    name = "out_params_test",
    srcs = ["out_params_test.cc"],
    deps = [
        ":out_params_cleartext",
        ":out_params_with_return_cleartext",
        ":single_out_param_cleartext",
        ":single_out_param_with_return_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

fhe_cc_library(
    name = "struct_cleartext",
    src = "struct.cc",
    hdrs = ["struct.h"],
    encryption = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "struct_tfhe",
    src = "struct.cc",
    hdrs = ["struct.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "struct_openfhe",
    src = "struct.cc",
    hdrs = ["struct.h"],
    encryption = "openfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "struct2_tfhe",
    src = "struct2.cc",
    hdrs = ["struct.h"],
    encryption = "tfhe",
    optimizer = "xls",
)

fhe_cc_library(
    name = "struct2_openfhe",
    src = "struct2.cc",
    hdrs = ["struct.h"],
    encryption = "openfhe",
    optimizer = "xls",
)

cc_test(
    name = "array_of_structs_cleartext_test",
    srcs = ["array_of_structs_cleartext_test.cc"],
    deps = [
        ":struct_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

cc_test(
    name = "array_of_structs_tfhe_test",
    srcs = ["array_of_structs_tfhe_test.cc"],
    deps = [
        ":struct_tfhe",
        ":tfhe_test_util",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

cc_test(
    name = "array_of_structs_openfhe_test",
    srcs = ["array_of_structs_openfhe_test.cc"],
    deps = [
        ":openfhe_test_util",
        ":struct_openfhe",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

cc_test(
    name = "array_of_scalars_cleartext_test",
    srcs = ["array_of_scalars_cleartext_test.cc"],
    deps = [
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

cc_test(
    name = "array_of_scalars_tfhe_test",
    srcs = ["array_of_scalars_tfhe_test.cc"],
    deps = [
        ":tfhe_test_util",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

cc_test(
    name = "array_of_scalars_openfhe_test",
    srcs = ["array_of_scalars_openfhe_test.cc"],
    deps = [
        ":openfhe_test_util",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/base",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

fhe_cc_library(
    name = "issue_36",
    src = "issue_36.cc",
    hdrs = ["issue_36.h"],
    encryption = "tfhe",
    interpreter = True,
    loop_unroll_rlimit = 500000,
    optimizer = "yosys",
)
