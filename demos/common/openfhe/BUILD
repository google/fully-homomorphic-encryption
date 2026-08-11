load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "timing_helper",
    srcs = ["timing_helper.cpp"],
    hdrs = ["timing_helper.h"],
    deps = [
        "@openfhe//:core",
        "@openfhe//:pke",
    ],
)
