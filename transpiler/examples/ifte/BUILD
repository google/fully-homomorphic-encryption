load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load(
    "//transpiler:parsers.bzl",
    "cc_to_xls_ir",
)
load(
    "//transpiler:fhe_xls.bzl",
    "xls_ir_to_bool_ir",
    "xls_ir_to_verilog",
)
load(
    "//transpiler:fhe_structs.bzl",
    "xls_cc_transpiled_structs",
)
load(
    "//transpiler:fhe.bzl",
    "cc_fhe_bool_ir_library",
    "cc_fhe_netlist_library",
)
load(
    "//transpiler:fhe_yosys.bzl",
    "verilog_to_netlist",
)
# load("//transpiler/rust:fhe_rs.bzl", "fhe_rust_library")
# load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

licenses(["notice"])

# Convert C++ to XLS IR and generate generic struct encodings

cc_to_xls_ir(
    name = "ifte_xls_ir",
    src = "ifte.cc",
    hdrs = ["ifte.h"],
)

# Generate struct encodings for each encryption scheme

xls_cc_transpiled_structs(
    name = "ifte_tfhe_structs",
    src = ":ifte_xls_ir",
    encryption = "tfhe",
)

xls_cc_transpiled_structs(
    name = "ifte_openfhe_structs",
    src = ":ifte_xls_ir",
    encryption = "openfhe",
)

xls_cc_transpiled_structs(
    name = "ifte_cleartext_structs",
    src = ":ifte_xls_ir",
    encryption = "cleartext",
)

# Generate optimized and booleanified IR (scheme-independent)

xls_ir_to_bool_ir(
    name = "ifte_optimized_booleanified_xls_ir",
    src = ":ifte_xls_ir",
    num_opt_passes = 1,
)

# Generate synthesizeable verilog from the XLS IR

xls_ir_to_verilog(
    name = "ifte_verilog",
    src = ":ifte_xls_ir",
)

# Generate netlists for each encryption scheme (these should be subsumed)

verilog_to_netlist(
    name = "ifte_tfhe_netlist",
    src = ":ifte_verilog",
    encryption = "tfhe",
)

verilog_to_netlist(
    name = "ifte_cleartext_netlist",
    src = ":ifte_verilog",
    encryption = "cleartext",
)

verilog_to_netlist(
    name = "ifte_openfhe_netlist",
    src = ":ifte_verilog",
    encryption = "openfhe",
)

# Generate C++ libraries for various combinations of schemes and interpretation modes.

cc_fhe_bool_ir_library(
    name = "ifte_xls_cleartext",
    src = ":ifte_optimized_booleanified_xls_ir",
    encryption = "cleartext",
    interpreter = False,
    transpiled_structs = ":ifte_cleartext_structs",
)

cc_fhe_netlist_library(
    name = "ifte_netlist_cleartext_interpreted",
    src = ":ifte_cleartext_netlist",
    encryption = "cleartext",
    interpreter = True,
    transpiled_structs = ":ifte_cleartext_structs",
)

cc_fhe_bool_ir_library(
    name = "ifte_xls_tfhe_interpreted",
    src = ":ifte_optimized_booleanified_xls_ir",
    encryption = "tfhe",
    interpreter = True,
    transpiled_structs = ":ifte_tfhe_structs",
)

cc_fhe_bool_ir_library(
    name = "ifte_xls_tfhe",
    src = ":ifte_optimized_booleanified_xls_ir",
    encryption = "tfhe",
    interpreter = False,
    transpiled_structs = ":ifte_tfhe_structs",
)

cc_fhe_bool_ir_library(
    name = "ifte_xls_openfhe",
    src = ":ifte_optimized_booleanified_xls_ir",
    encryption = "openfhe",
    interpreter = False,
    transpiled_structs = ":ifte_openfhe_structs",
)

cc_fhe_bool_ir_library(
    name = "ifte_xls_openfhe_interpreted",
    src = ":ifte_optimized_booleanified_xls_ir",
    encryption = "openfhe",
    interpreter = True,
    transpiled_structs = ":ifte_openfhe_structs",
)

cc_fhe_netlist_library(
    name = "ifte_netlist_tfhe_interpreted",
    src = ":ifte_tfhe_netlist",
    encryption = "tfhe",
    interpreter = True,
    transpiled_structs = ":ifte_tfhe_structs",
)

cc_fhe_netlist_library(
    name = "ifte_netlist_openfhe_interpreted",
    src = ":ifte_openfhe_netlist",
    encryption = "openfhe",
    interpreter = True,
    transpiled_structs = ":ifte_openfhe_structs",
)

cc_binary(
    name = "ifte_cleartext_testbench",
    srcs = ["ifte_cleartext_testbench.cc"],
    deps = [
        ":ifte_xls_cleartext",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "ifte_yosys_interpreted_cleartext_testbench",
    srcs = ["ifte_cleartext_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_CLEARTEXT"],
    deps = [
        ":ifte_netlist_cleartext_interpreted",
        "//transpiler/data:cleartext_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
    ],
)

cc_binary(
    name = "ifte_tfhe_testbench",
    srcs = ["ifte_tfhe_testbench.cc"],
    deps = [
        ":ifte_xls_tfhe",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "ifte_yosys_interpreted_tfhe_testbench",
    srcs = ["ifte_tfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_TFHE"],
    deps = [
        ":ifte_netlist_tfhe_interpreted",
        "//transpiler/data:tfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@tfhe//:libtfhe",
    ],
)

cc_binary(
    name = "ifte_yosys_interpreted_openfhe_testbench",
    srcs = ["ifte_openfhe_testbench.cc"],
    copts = ["-DUSE_YOSYS_INTERPRETED_OPENFHE"],
    deps = [
        ":ifte_netlist_openfhe_interpreted",
        "//transpiler/data:openfhe_data",
        "@com_google_absl//absl/base",
        "@com_google_xls//xls/common/logging",
        "@com_google_xls//xls/common/status:status_macros",
        "@openfhe//:binfhe",
    ],
)

# fhe_rust_library(
#     name = "ifte_rs_fhe_lib",
#     src = "ifte.cc",
#     hdrs = ["ifte.h"],
# )
#
# rust_library(
#     name = "ifte_rs_lib",
#     srcs = ["ifte_rs_lib.rs"],
#     deps = [
#         ":ifte_rs_fhe_lib",
#         "@crate_index//:tfhe",
#     ],
# )
#
# rust_test(
#     name = "ifte_rs_test",
#     size = "large",
#     crate = ":ifte_rs_lib",
#     tags = ["nofastbuild"],
# )
