load("//transpiler:fhe.bzl", "cc_fhe_bool_ir_library", "cc_to_xls_ir", "xls_cc_transpiled_structs", "xls_ir_to_bool_ir")

licenses(["notice"])

_FHE_SCHEMES = [
    "tfhe",
    "openfhe",
    "cleartext",
]

# These are the structs defined in primitives.h.  The list here must match the
# set of definitions in primitives.h, to make sure we unwrap them when
# transpiling them.
_PRIMITIVES = [
    "PrimitiveBool",
    "PrimitiveChar",
    "PrimitiveSignedChar",
    "PrimitiveUnsignedChar",
    "PrimitiveSignedShort",
    "PrimitiveUnsignedShort",
    "PrimitiveSignedInt",
    "PrimitiveUnsignedInt",
    "PrimitiveSignedLong",
    "PrimitiveUnsignedLong",
]

# Convert C++ to XLS IR

cc_to_xls_ir(
    name = "primitives_xls_ir",
    src = "primitives.cc",
    hdrs = ["primitives.h"],
    unwrap = _PRIMITIVES,
)

# Generate struct encodings for each encryption scheme from the XLS IR and its metadata.

[
    [
        xls_cc_transpiled_structs(
            name = "primitives_transpiled_structs_" + scheme,
            src = ":primitives_xls_ir",
            encryption = scheme,
            unwrap = _PRIMITIVES,
        ),
    ]
    for scheme in _FHE_SCHEMES
]

# Generate optimized and booleanified IR (scheme-independent)

xls_ir_to_bool_ir(
    name = "primitives_optimized_booleanified_xls_ir",
    src = ":primitives_xls_ir",
    num_opt_passes = 1,
)

# Generate the C++ libraries for each scheme

[
    [
        cc_fhe_bool_ir_library(
            name = "primitives_" + scheme,
            src = ":primitives_optimized_booleanified_xls_ir",
            encryption = scheme,
            interpreter = False,
            skip_scheme_data_deps = True,
            transpiled_structs = ":primitives_transpiled_structs_" + scheme,
        ),
    ]
    for scheme in _FHE_SCHEMES
]

cc_library(
    name = "cleartext_value",
    hdrs = ["cleartext_value.h"],
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/types:span",
    ],
)

cc_library(
    name = "tfhe_value",
    hdrs = ["tfhe_value.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":cleartext_value",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/types:span",
        "@tfhe//:libtfhe",
    ],
)

cc_library(
    name = "openfhe_value",
    hdrs = ["openfhe_value.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":cleartext_value",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/types:span",
        "@openfhe//:binfhe",
    ],
)

cc_library(
    name = "cleartext_data",
    hdrs = ["cleartext_data.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":cleartext_value",
        ":primitives_cleartext",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/types:span",
    ],
)

cc_library(
    name = "tfhe_data",
    hdrs = ["tfhe_data.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":cleartext_data",
        ":cleartext_value",
        ":primitives_tfhe",
        ":tfhe_value",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/types:span",
        "@tfhe//:libtfhe",
    ],
)

cc_library(
    name = "openfhe_data",
    hdrs = ["openfhe_data.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":cleartext_data",
        ":cleartext_value",
        ":openfhe_value",
        ":primitives_openfhe",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/types:span",
        "@openfhe//:binfhe",
    ],
)

cc_test(
    name = "cleartext_data_test",
    srcs = ["cleartext_data_test.cc"],
    deps = [
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

cc_test(
    name = "tfhe_data_test",
    srcs = ["tfhe_data_test.cc"],
    deps = [
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)

cc_test(
    name = "openfhe_data_test",
    srcs = ["openfhe_data_test.cc"],
    deps = [
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)
