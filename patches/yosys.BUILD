load("@rules_foreign_cc//foreign_cc:defs.bzl", "make")

make(
    name = "yosys",
    args = [
         "-j16",
         "CONFIG=gcc",
    ],
    env = {
        "ENABLE_TCL": "0",
        "ENABLE_GLOB": "0",
        "ENABLE_PLUGINS": "0",
        "ENABLE_READLINE": "0",
        "ENABLE_COVER": "0",
        "ENABLE_ZLIB": "0",
        "ENABLE_ABC": "1",
        "ABCEXTERNAL": "$(location @abc//:abc_bin)",
    },
    targets = [ "yosys" ],
    data = [
        "@abc//:abc_bin",
    ],
    out_binaries = [ "yosys" ],
    out_include_dir = "",
    out_lib_dir = "",
    lib_source = ":yosys_srcs",
    postfix_script = "cp $EXT_BUILD_ROOT/external/yosys/yosys $INSTALLDIR/bin/yosys",
    visibility = ["//visibility:public"],
)

filegroup(
    name = "yosys_bin",
    srcs = [":yosys"],
    output_group = "yosys",
    visibility = ["//visibility:public"],
)

filegroup(
    name = "yosys_srcs",
    srcs = glob(["*", "**/*"])
)
