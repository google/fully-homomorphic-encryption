load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

package(
    default_visibility = ["//visibility:private"],
)

CMAKE_BUILD_PARALLEL_LEVEL = "24"

CACHE_ENTRIES = {
    "BUILD_BENCHMARKS": "OFF",
    "BUILD_UNITTESTS": "OFF",
    "CMAKE_BUILD_TYPE": "Release",
    "CMAKE_C_COMPILER": "clang-10",
    "CMAKE_CXX_COMPILER": "clang++-10",
    "WITH_NATIVEOPT": "ON",
    # For 128-bit classical security, one can reduce NATIVE_SIZE from 64 to 32,
    # and get a marginal speedup in return. The moduli required to achieve
    # 128-bit quantum security, or classical security higher than 128 bits, do
    # not fit in a 32-bit word. By default we choose the more general option.
    "NATIVE_SIZE": "64",
}

ENV = {
    "CMAKE_BUILD_PARALLEL_LEVEL": CMAKE_BUILD_PARALLEL_LEVEL,
}

DEFINES = ["MATHBACKEND=2"]

cmake(
    name = "core",
    cache_entries = CACHE_ENTRIES,
    defines = DEFINES,
    env = ENV,
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
    cache_entries = CACHE_ENTRIES,
    defines = DEFINES,
    env = ENV,
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
