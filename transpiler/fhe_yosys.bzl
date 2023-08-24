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

_NETLIST_ANALYZER = "//transpiler/tools:netlist_analyzer_main"

_YOSYS = "@yosys//:yosys_bin"
_ABC = "@abc//:abc_bin"

_LUT_TO_LUTMUX_SCRIPTS = {
    0: "//transpiler/yosys:map_lut_to_lutmux.v",
    2: "//transpiler/yosys:map_lut_to_lutmux2.v",
    3: "//transpiler/yosys:map_lut_to_lutmux3.v",
    4: "//transpiler/yosys:map_lut_to_lutmux4.v",
    5: "//transpiler/yosys:map_lut_to_lutmux5.v",
    6: "//transpiler/yosys:map_lut_to_lutmux6.v",
}
VALID_LUT_SIZES = {
    "yosys": [0, 2, 3, 4, 5, 6],
    # No other optimizers currently support LUTs, so they should all be [0]
    "xls": [0],
}

_YOSYS_SCRIPT_TEMPLATE_NO_LUT = """cat>{script}<<EOF
read_verilog {verilog}
hierarchy -check -top $(cat {entry})
proc
memory
techmap; opt
abc -liberty {cell_library}
opt_clean -purge
hierarchy -generate * o:Y o:Q i:*
torder -stop DFFSR Q
clean
write_verilog {netlist_path}
show -format dot -prefix {netlist_path} -viewer touch
EOF
"""

# In the script below, putting a second `write_verilog` command before the
# `techmap` command is required to make a visualization compact and useful. The
# output of the techmap results in the wires organized in such a way that yosys
# doesn't display the circuit in nice layers.
_YOSYS_SCRIPT_TEMPLATE_LUT = """cat>{script}<<EOF
read_verilog {verilog}
hierarchy -check -top $(cat {entry})
proc
memory
techmap; opt
abc -lut {lut_size}
opt_clean -purge
write_verilog -attr2comment {netlist_path}.pre_techmap
show -format dot -prefix {netlist_path} -viewer touch
techmap -map {lutmap_script}
opt_clean -purge
hierarchy -generate * o:Y o:Q i:*
torder -stop DFFSR Q
clean
write_verilog -noexpr -attr2comment {netlist_path}
EOF
"""

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
    library_name = src[VerilogInfo].library_name
    stem = src[VerilogInfo].stem

    name = stem + "_" + ctx.attr.encryption
    if stem != library_name:
        name = library_name
    generated_files = _generate_netlist(ctx, name, verilog_ir_file, metadata_entry_file)
    netlist_file = generated_files[0]
    netlist_analyzer_files = _generate_netlist_analysis(ctx, name, netlist_file)
    outputs = generated_files
    return [
        DefaultInfo(files = depset(outputs + netlist_analyzer_files + src[DefaultInfo].files.to_list())),
        BooleanifiedIrOutputInfo(
            ir = depset([netlist_file]),
            metadata = depset([metadata_file]),
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
        "lut_size": attr.int(
            doc = """
            Whether to use look-up tables (LUTs) in place of standard boolean
            gates in the output circuit. Using lookup tables makes the
            resulting circuit smaller, but not all cryptographic backends
            support programmable bootstrapping of the width required to use
            LUTs of a given size.

            A value of zero means that LUTs should not be used.
            """,
            values = VALID_LUT_SIZES["yosys"],
            default = 0,
        ),
        "lutmap_script": attr.label(
            doc = """
            A verilog techmap script for converting LUT expressions to liberty cells.
            Unused if lut_size is unset.
            """,
            allow_single_file = [".v"],
        ),
        "_netlist_analyzer": attr.label(
            default = Label(_NETLIST_ANALYZER),
            executable = True,
            cfg = "exec",
        ),
        "_yosys": executable_attr(_YOSYS),
        "_abc": executable_attr(_ABC),
    },
)

def verilog_to_netlist(name, src, encryption, cell_library = None, lut_size = 0):
    cell_library = cell_library or FHE_ENCRYPTION_SCHEMES[encryption]
    if encryption in FHE_ENCRYPTION_SCHEMES:
        _verilog_to_netlist(
            name = name,
            src = src,
            encryption = encryption,
            cell_library = cell_library,
            lut_size = lut_size,
            # unused if lut_size is not set
            lutmap_script = _LUT_TO_LUTMUX_SCRIPTS[lut_size],
        )
    else:
        fail("Invalid encryption value:", encryption)

def _generate_yosys_script(ctx, stem, verilog, netlist_path, entry):
    ys_script = ctx.actions.declare_file("%s.ys" % stem)
    dot_visualization = ctx.actions.declare_file("%s.netlist.v.dot" % stem)
    additional_files = [dot_visualization]

    if ctx.attr.lut_size:
        additional_files.append(ctx.actions.declare_file("%s.netlist.v.pre_techmap" % stem))
        sh_cmd = _YOSYS_SCRIPT_TEMPLATE_LUT.format(
            entry = entry.path,
            lut_size = ctx.attr.lut_size,
            lutmap_script = ctx.file.lutmap_script.path,
            netlist_path = netlist_path,
            script = ys_script.path,
            verilog = verilog.path,
        )
    else:
        sh_cmd = _YOSYS_SCRIPT_TEMPLATE_NO_LUT.format(
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

    return ys_script, additional_files

def _generate_netlist_analysis(ctx, stem, netlist):
    netlist_analysis_result = ctx.actions.declare_file("%s.netlist.v.analysis.txt" % stem)
    args = [
        "-cell_library",
        ctx.file.cell_library.path,
        "-netlist",
        netlist.path,
        "-output_path",
        netlist_analysis_result.path,
    ]
    ctx.actions.run(
        inputs = [netlist, ctx.file.cell_library],
        outputs = [netlist_analysis_result],
        arguments = args,
        executable = ctx.executable._netlist_analyzer,
        toolchain = None,
    )
    return [netlist_analysis_result]

def _generate_netlist(ctx, stem, verilog, entry):
    netlist = ctx.actions.declare_file("%s.netlist.v" % stem)

    script, additional_files = _generate_yosys_script(ctx, stem, verilog, netlist.path, entry)

    yosys_runfiles_dir = ctx.executable._yosys.path + ".runfiles"

    args = ctx.actions.args()
    args.add("-q")  # quiet mode only errors printed to stderr
    args.add("-q")  # second q don't print warnings
    args.add("-Q")  # Don't print header
    args.add("-T")  # Don't print footer
    args.add_all("-s", [script.path])  # command execution

    ctx.actions.run(
        inputs = [verilog, script],
        outputs = [netlist] + additional_files,
        arguments = [args],
        executable = ctx.executable._yosys,
        tools = [
            ctx.file.cell_library,
            ctx.file.lutmap_script,
            ctx.executable._abc,
        ],
        env = {
            "YOSYS_DATDIR": yosys_runfiles_dir + "/yosys/share/yosys",
        },
        toolchain = None,
    )

    return [netlist, script] + additional_files
