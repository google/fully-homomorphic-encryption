# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Common routines for working with Yosys netlists in bazel files."""

load(
    "//transpiler:fhe_common.bzl",
    "BooleanifiedIrInfo",
    "BooleanifiedIrOutputInfo",
    "FHE_ENCRYPTION_SCHEMES",
    "VerilogInfo",
    "VerilogOutputInfo",
    "executable_attr",
)

_YOSYS = "@yosys//:yosys_bin"
_ABC = "@abc//:abc_bin"

NetlistEncryptionInfo = provider(
    """Passes along the encryption attribute.""",
    fields = {
        "encryption": "Encryption scheme used",
    },
)

def _verilog_to_netlist_impl(ctx):
    src = ctx.attr.src
    metadata_file = src[VerilogOutputInfo].metadata.to_list()[0]
    metadata_entry_file = src[VerilogOutputInfo].metadata_entry.to_list()[0]
    verilog_ir_file = src[VerilogOutputInfo].verilog_ir_file.to_list()[0]
    generic_struct_header = src[VerilogOutputInfo].generic_struct_header.to_list()[0]
    library_name = src[VerilogInfo].library_name
    stem = src[VerilogInfo].stem

    name = stem + "_" + ctx.attr.encryption
    if stem != library_name:
        name = library_name
    netlist_file, yosys_script_file = _generate_netlist(ctx, name, verilog_ir_file, metadata_entry_file)

    outputs = [netlist_file, yosys_script_file]
    return [
        DefaultInfo(files = depset(outputs + src[DefaultInfo].files.to_list())),
        BooleanifiedIrOutputInfo(
            ir = depset([netlist_file]),
            metadata = depset([metadata_file]),
            generic_struct_header = depset([generic_struct_header]),
            hdrs = depset(src[VerilogOutputInfo].hdrs.to_list()),
        ),
        BooleanifiedIrInfo(
            library_name = library_name,
            stem = stem,
            optimizer = "yosys",
        ),
        NetlistEncryptionInfo(
            encryption = ctx.attr.encryption,
        ),
    ]

_verilog_to_netlist = rule(
    doc = """
      This rule takes XLS IR output by XLScc, and converts it to a Verilog
      netlist using the basic primitives defined in a cell library.
      """,
    implementation = _verilog_to_netlist_impl,
    attrs = {
        "src": attr.label(
            providers = [VerilogOutputInfo, VerilogInfo],
            doc = "A single XLS IR source file (emitted by XLScc).",
            mandatory = True,
        ),
        "encryption": attr.string(
            doc = """
            FHE encryption scheme used by the resulting program. Choices are
            {tfhe, openfhe, cleartext}. 'cleartext' means the program runs in cleartext,
            skipping encryption; this has zero security, but is useful for debugging.
            """,
            values = FHE_ENCRYPTION_SCHEMES.keys(),
            default = "tfhe",
        ),
        "cell_library": attr.label(
            doc = "A single cell-definition library in Liberty format.",
            allow_single_file = [".liberty"],
        ),
        "_yosys": executable_attr(_YOSYS),
        "_abc": executable_attr(_ABC),
    },
)

def verilog_to_netlist(name, src, encryption):
    if encryption in FHE_ENCRYPTION_SCHEMES:
        _verilog_to_netlist(name = name, src = src, encryption = encryption, cell_library = FHE_ENCRYPTION_SCHEMES[encryption])
    else:
        fail("Invalid encryption value:", encryption)

def _generate_yosys_script(ctx, stem, verilog, netlist_path, entry):
    ys_script = ctx.actions.declare_file("%s.ys" % stem)
    sh_cmd = """cat>{script}<<EOF
# read_verilog -sv {verilog} # if we want to use SV
read_verilog {verilog}
hierarchy -check -top $(cat {entry})
proc; opt
flatten; opt
techmap; opt
abc -liberty {cell_library}
opt_clean -purge
clean
write_verilog {netlist_path}
EOF
    """.format(
        script = ys_script.path,
        verilog = verilog.path,
        entry = entry.path,
        cell_library = ctx.file.cell_library.path,
        netlist_path = netlist_path,
    )

    ctx.actions.run_shell(
        inputs = [entry],
        outputs = [ys_script],
        command = sh_cmd,
    )

    return ys_script

def _generate_netlist(ctx, stem, verilog, entry):
    netlist = ctx.actions.declare_file("%s.netlist.v" % stem)

    script = _generate_yosys_script(ctx, stem, verilog, netlist.path, entry)

    yosys_runfiles_dir = ctx.executable._yosys.path + ".runfiles"

    args = ctx.actions.args()
    args.add("-q")  # quiet mode only errors printed to stderr
    args.add("-q")  # second q don't print warnings
    args.add("-Q")  # Don't print header
    args.add("-T")  # Don't print footer
    args.add_all("-s", [script.path])  # command execution

    ctx.actions.run(
        inputs = [verilog, script],
        outputs = [netlist],
        arguments = [args],
        executable = ctx.executable._yosys,
        tools = [ctx.file.cell_library, ctx.executable._abc],
        env = {
            "YOSYS_DATDIR": yosys_runfiles_dir + "/yosys/share/yosys",
        },
    )

    return (netlist, script)
