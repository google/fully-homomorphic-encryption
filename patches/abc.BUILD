load("@rules_foreign_cc//foreign_cc:defs.bzl", "make")

package(
    default_visibility = ["//visibility:private"],
)

make(
    name = "abc",
    args = [
        "-j16",
    ],
    copts = [
        "-Dredacted=0",
    ],
    lib_source = ":abc_srcs",
    out_binaries = ["abc"],
    out_include_dir = "",
    out_lib_dir = "",
    postfix_script = "cp abc $INSTALLDIR/bin/abc",
    targets = ["abc"],
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
    srcs = glob([
        "*",
        "**/*",
    ]),
)
