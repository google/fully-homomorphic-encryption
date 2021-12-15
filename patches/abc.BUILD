load("@rules_foreign_cc//foreign_cc:defs.bzl", "make")

package(
    default_visibility = ["//visibility:private"],
)

make(
    name = "abc",
    args = [
        "-j16",
    ],
    env = {
        "CMAKE_C_FLAGS": "-Dredacted=0",
    },
    targets = [ "abc" ],
    out_binaries = [ "abc" ],
    out_include_dir = "",
    out_lib_dir = "",
    lib_source = ":abc_srcs",
    postfix_script = "cp $EXT_BUILD_ROOT/external/abc/abc $INSTALLDIR/bin/abc",
    visibility = ["//visibility:public"],
)

filegroup(
    name = "abc_bin",
    srcs = [":abc"],
    output_group = "abc",
    visibility = ["//visibility:public"],
)

filegroup(
    name = "abc_srcs",
    srcs = glob(["*", "**/*"])
)
