load("//transpiler:fhe.bzl", "fhe_cc_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

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
            backend = "tfhe",
            num_opt_passes = 2,
            optimizer = "xls",
        ),
        fhe_cc_library(
            name = "%s_palisade" % i,
            src = "%s.cc" % i,
            hdrs = [],
            backend = "palisade",
            num_opt_passes = 2,
            optimizer = "xls",
        ),
    ]
    for i in _PRIMITIVE_TYPES
]

cc_library(
    name = "test_util",
    testonly = True,
    srcs = ["test_util.cc"],
    hdrs = ["test_util.h"],
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
    name = "palisade_test_util",
    testonly = True,
    srcs = ["palisade_test_util.cc"],
    hdrs = ["palisade_test_util.h"],
    visibility = [
        "//:__subpackages__",
    ],
    deps = [
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest",
        "@com_google_xls//xls/common/status:matchers",
        "@palisade//:binfhe",
    ],
)

cc_test(
    name = "types_test",
    srcs = ["types_test.cc"],
    shard_count = 4,
    deps = [":%s_tfhe" % i for i in _PRIMITIVE_TYPES] + [
        ":test_util",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:tfhe_data",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "palisade_types_test",
    srcs = ["palisade_types_test.cc"],
    shard_count = 4,
    deps = [":%s_palisade" % i for i in _PRIMITIVE_TYPES] + [
        ":palisade_test_util",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:palisade_data",
        "@palisade//:binfhe",
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
            backend = "tfhe",
            num_opt_passes = 2,
            optimizer = "xls",
        ),
        fhe_cc_library(
            name = "%s_palisade" % i,
            src = "%s.cc" % i,
            hdrs = [],
            backend = "palisade",
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
        ":test_util",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:tfhe_data",
        "@tfhe//:libtfhe",
    ],
)

cc_test(
    name = "palisade_control_structure_test",
    size = "medium",
    srcs = ["palisade_control_structure_test.cc"],
    shard_count = 8,
    deps = [":%s_palisade" % i for i in _CONTROL_STRUCTURE] + [
        ":palisade_test_util",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/strings",
        "@com_google_xls//xls/common/status:matchers",
        "//transpiler/data:palisade_data",
        "@palisade//:binfhe",
    ],
)

fhe_cc_library(
    name = "function_cleartext",
    src = "function.cc",
    hdrs = [],
    backend = "cleartext",
    num_opt_passes = 1,
    optimizer = "xls",
)

fhe_cc_library(
    name = "out_params_cleartext",
    src = "out_params.cc",
    hdrs = [],
    backend = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "out_params_with_return_cleartext",
    src = "out_params_with_return.cc",
    hdrs = [],
    backend = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "single_out_param_cleartext",
    src = "single_out_param.cc",
    hdrs = [],
    backend = "cleartext",
    optimizer = "xls",
)

fhe_cc_library(
    name = "single_out_param_with_return_cleartext",
    src = "single_out_param_with_return.cc",
    hdrs = [],
    backend = "cleartext",
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
        "//transpiler/data:boolean_data",
        "@com_google_absl//absl/base",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)
