# BUILD file for a bazel-native OpenFHE build

package(
    default_visibility = ["//visibility:public"],
    features = [
        "-layering_check",  # Incompatible with `#include "gtest/gtest.h"`
        "-use_header_modules",  # Incompatible with -fexceptions.
    ],
)

licenses(["notice"])

# This rule exists so that the python frontend can get access to the headers to
# pass dynamically to clang when building compiled code.
filegroup(
    name = "headers",
    srcs = glob([
        "src/binfhe/include/**/*.h",
        "src/core/include/**/*.h",
        "src/pke/include/**/*.h",
    ]),
)

cc_library(
    name = "core",
    srcs = glob([
        "src/core/lib/**/*.c",
        "src/core/lib/**/*.cpp",
    ]),
    copts =  [
        # /utils/blockAllocator/blockAllocator.cpp has misaligned-pointer-use
        "-Wno-non-virtual-dtor",
        "-Wno-shift-op-parentheses",
        "-Wno-unused-private-field",
        "-fexceptions",
        "-fno-sanitize=alignment",
    ],
    defines = [
        "MATHBACKEND=2",
        "OPENFHE_VERSION=1.11.3"
    ],
    includes = [
        "src/core/include",
        "src/core/lib",
    ],
    linkopts = [
    ],
    textual_hdrs = glob([
        "src/core/include/**/*.h",
        "src/core/lib/**/*.cpp",
    ]),
    deps = ["@cereal"],
)

cc_library(
    name = "binfhe",
    srcs = glob([
        "src/binfhe/lib/**/*.cpp",
    ]),
    copts = [
        "-Wno-non-virtual-dtor",
        "-Wno-shift-op-parentheses",
        "-Wno-unused-private-field",
        "-fexceptions",
    ],
    defines = [
        "MATHBACKEND=2",
        "OPENFHE_VERSION=1.11.3"
    ],
    includes = [
        "src/binfhe/include",
        "src/binfhe/lib",
    ],
    linkopts = [
    ],
    textual_hdrs = glob(["src/binfhe/include/**/*.h"]),
    deps = [
        "@openfhe//:core",
    ],
)