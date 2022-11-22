load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

package(
    default_visibility = ["//visibility:private"],
)

cmake(
    name = "core",
    cache_entries = {
        "BUILD_BENCHMARKS": "OFF",
        "BUILD_UNITTESTS": "OFF",
        "CMAKE_BUILD_TYPE": "Release",
    },
    defines = ["MATHBACKEND=2"],
    env = {
        "CMAKE_BUILD_PARALLEL_LEVEL": "16",
    },
    includes = [
        "openfhe",
        "openfhe/core",
    ],
    lib_source = ":openfhe_srcs",
    out_include_dir = "include",
    out_shared_libs = [
        "libOPENFHEcore.so.1",
    ],
    visibility = ["//visibility:public"],
)

cmake(
    name = "binfhe",
    cache_entries = {
        "BUILD_BENCHMARKS": "OFF",
        "BUILD_UNITTESTS": "OFF",
        "CMAKE_BUILD_TYPE": "Release",
    },
    defines = ["MATHBACKEND=2"],
    env = {
        "CMAKE_BUILD_PARALLEL_LEVEL": "16",
    },
    includes = [
        "openfhe",
        "openfhe/binfhe",
        "openfhe/core",
    ],
    lib_source = ":openfhe_srcs",
    out_include_dir = "include",
    out_shared_libs = [
        "libOPENFHEbinfhe.so.1",
        "libOPENFHEcore.so.1",
    ],
    visibility = ["//visibility:public"],
)

# Private rules follow below.

filegroup(
    name = "openfhe_srcs",
    srcs = glob([
        "*",
        "configure/**",
        "src/**",
        "third-party/**",
    ]),
)
