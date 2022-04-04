licenses(["notice"])

cc_library(
    name = "boolean_data",
    hdrs = ["boolean_data.h"],
    visibility = ["//visibility:public"],
    deps = [
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
        ":boolean_data",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/types:span",
        "@tfhe//:libtfhe",
    ],
)

cc_library(
    name = "palisade_data",
    hdrs = ["palisade_data.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":boolean_data",
        "@com_google_absl//absl/container:fixed_array",
        "@com_google_absl//absl/types:span",
        "@palisade//:binfhe",
    ],
)

cc_test(
    name = "boolean_data_test",
    srcs = ["boolean_data_test.cc"],
    deps = [
        "//transpiler/data:boolean_data",
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
    name = "palisade_data_test",
    srcs = ["palisade_data_test.cc"],
    deps = [
        "//transpiler/data:palisade_data",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
        "@com_google_xls//xls/common/status:matchers",
    ],
)
