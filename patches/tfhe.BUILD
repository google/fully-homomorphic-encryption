load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

package(
    default_visibility = ["//visibility:private"],
)

cmake(
    name = "libtfhe",
    cache_entries = {
        "CMAKE_CXX_FLAGS": "-Wno-unused-but-set-parameter -Wno-restrict -std=c++17",
    },
    lib_source = "@tfhe//:srcs",
    out_shared_libs = ["libtfhe-nayuki-portable.so"],
    visibility = ["//visibility:public"],
)

# Private rules follow below.

filegroup(
    name = "srcs",
    srcs = glob(["src/**"]),
)
