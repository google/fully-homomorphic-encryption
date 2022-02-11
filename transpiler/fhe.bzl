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

"""Rules for generating FHE-C++."""

_FHE_TRANSPILER = "//transpiler"
_STRUCT_HEADER_GENERATOR = "//transpiler/struct_transpiler:struct_transpiler"
_XLSCC = "@com_google_xls//xls/contrib/xlscc:xlscc"
_XLS_BOOLEANIFY = "@com_google_xls//xls/tools:booleanify_main"
_XLS_OPT = "@com_google_xls//xls/tools:opt_main"
_GET_TOP_FUNC_FROM_PROTO = "@com_google_xls//xls/contrib/xlscc:get_top_func_from_proto"
_XLS_CODEGEN = "@com_google_xls//xls/tools:codegen_main"

_YOSYS = "@yosys//:yosys_bin"
_ABC = "@abc//:abc_bin"
_TFHE_CELLS_LIBERTY = "//transpiler:tfhe_cells.liberty"

def _run(ctx, inputs, out_ext, tool, args, entry = None):
    """A helper to run a shell script and capture the output.

    ctx:  The blaze context.
    inputs: A list of files used by the shell.
    out_ext: An extension to add to the current label for the output file.
    tool: What tool to run.
    args: A list of arguments to pass to the tool.
    entry: If specified, it points to a file contianing the entry point; that
           information is extracted and provided as value to the --entry
           command-line switch.

    Returns:
      The File output.
    """
    library_name = ctx.attr.library_name or ctx.label.name
    out = ctx.actions.declare_file("%s%s" % (library_name, out_ext))
    arguments = " ".join(args)
    if entry != None:
        arguments += " --entry $(cat {})".format(entry.path)
    ctx.actions.run_shell(
        inputs = inputs,
        outputs = [out],
        tools = [tool],
        command = "%s %s > %s" % (tool.path, arguments, out.path),
    )
    return out

def _get_top_func(ctx, metadata_file):
    """Extract the name of the entry function from the metadata file."""
    return _run(
        ctx,
        [metadata_file],
        ".entry",
        ctx.executable._get_top_func_from_proto,
        [metadata_file.path],
    )

def _build_ir(ctx):
    """Build the XLS IR from a C++ source.

    Args:
      ctx: The Blaze context.

    Returns:
      A File containing the generated IR and one containing metadata about
      the translated function (signature, etc.).
    """
    library_name = ctx.attr.library_name or ctx.label.name
    ir_file = ctx.actions.declare_file("%s.ir" % library_name)
    metadata_file = ctx.actions.declare_file("%s_meta.proto" % library_name)
    ctx.actions.run_shell(
        inputs = [ctx.file.src] + ctx.files.hdrs,
        outputs = [ir_file, metadata_file],
        tools = [ctx.executable._xlscc],
        command = "%s %s -meta_out %s > %s" % (
            ctx.executable._xlscc.path,
            ctx.file.src.path,
            metadata_file.path,
            ir_file.path,
        ),
    )
    return (ir_file, metadata_file, _get_top_func(ctx, metadata_file))

def _optimize_ir(ctx, src, extension, entry, options = []):
    """Optimize XLS IR."""
    return _run(ctx, [src, entry], extension, ctx.executable._xls_opt, [src.path] + options, entry)

def _booleanify_ir(ctx, src, extension, entry):
    """Optimize XLS IR."""
    return _run(ctx, [src, entry], extension, ctx.executable._xls_booleanify, ["--ir_path", src.path], entry)

def _optimize_and_booleanify_repeatedly(ctx, ir_file, entry):
    """Runs several passes of optimization followed by booleanification.

    Returns [%.opt.ir, %.opt.bool.ir, %.opt.bool.opt.ir, %.opt.bool.opt.bool.ir, ...]
    """
    results = [ir_file]
    suffix = ""

    # With zero optimization passes, we still want to run the optimizer with an
    # inlining pass, as the booleanifier expects a single function.
    if ctx.attr.num_opt_passes == 0:
        suffix += ".opt"
        results.append(_optimize_ir(ctx, results[-1], suffix + ".ir", entry, ["--run_only_passes=inlining"]))
        suffix += ".bool"
        results.append(_booleanify_ir(ctx, results[-1], suffix + ".ir", entry))
    else:
        for i in range(ctx.attr.num_opt_passes):
            suffix += ".opt"
            results.append(_optimize_ir(ctx, results[-1], suffix + ".ir", entry))
            suffix += ".bool"
            results.append(_booleanify_ir(ctx, results[-1], suffix + ".ir", entry))
    return results[1:]

def _pick_last_bool_file(optimized_files):
    """ Pick the last booleanifed IR file from a list of file produced by _optimize_and_booleanify_repeatedly().

    The last %.*.bool.ir file may or may not be the smallest one.  For some IR
    inputs, an additional optimization/booleanification pass results in a
    larger file.  This is why we have num_opt_passes.
    """

    # structure is [%.opt.ir, %.opt.bool.ir, %.opt.bool.opt.ir,
    # %.opt.bool.opt.bool.ir, ...], so every other file is the result of an
    # optimization + booleanification pass.
    inspect = optimized_files[1::2]
    return optimized_files[-1]

def _fhe_transpile_ir(ctx, src, metadata, entry, optimizer, backend):
    """Transpile XLS IR into C++ source."""
    library_name = ctx.attr.library_name or ctx.label.name
    out_cc = ctx.actions.declare_file("%s.cc" % library_name)
    out_h = ctx.actions.declare_file("%s.h" % library_name)

    args = [
        "-ir_path",
        src.path,
        "-metadata_path",
        metadata.path,
        "-cc_path",
        out_cc.path,
        "-header_path",
        out_h.path,
        "-optimizer",
        optimizer,
        "-backend",
        backend,
    ]

    if optimizer == "yosys":
        args += ["-liberty_path", ctx.file.cell_library.path]

    ctx.actions.run(
        inputs = [src, metadata, ctx.file.cell_library],
        outputs = [out_cc, out_h],
        executable = ctx.executable._fhe_transpiler,
        arguments = args,
    )
    return [out_cc, out_h]

def _generate_struct_header(ctx, metadata, optimizer, backend):
    """Transpile XLS IR into C++ source."""
    library_name = ctx.attr.library_name or ctx.label.name
    generic_struct_h = ctx.actions.declare_file("%s.generic.types.h" % library_name)
    specific_struct_h = ctx.actions.declare_file("%s.types.h" % library_name)

    args = [
        "-metadata_path",
        metadata.path,
        "-original_headers",
        ",".join([hdr.path for hdr in ctx.files.hdrs]),
        "-output_path",
        generic_struct_h.path,
    ]
    if optimizer == "yosys":
        args.append("-struct_fields_in_declaration_order")
    ctx.actions.run(
        inputs = [metadata],
        outputs = [generic_struct_h],
        executable = ctx.executable._struct_header_generator,
        arguments = args,
    )

    backend_type = backend
    if backend_type.startswith("interpreted_"):
        backend_type = backend_type[len("interpreted_"):]
    args = [
        "-metadata_path",
        metadata.path,
        "-output_path",
        specific_struct_h.path,
        "-generic_header_path",
        generic_struct_h.path,
        "-backend_type",
        backend_type,
    ]
    ctx.actions.run(
        inputs = [metadata, generic_struct_h],
        outputs = [specific_struct_h],
        executable = ctx.executable._struct_header_generator,
        arguments = args,
    )

    return [generic_struct_h, specific_struct_h]

def _generate_verilog(ctx, src, extension, entry):
    """Convert optimized XLS IR to Verilog."""
    return _run(
        ctx,
        [src, entry],
        extension,
        ctx.executable._xls_codegen,
        [
            src.path,
            "--delay_model=unit",
            "--clock_period_ps=1000",
            "--generator=combinational",
            "--use_system_verilog=false",  # edit the YS script if this changes
        ],
        entry,
    )

def _generate_yosys_script(ctx, verilog, netlist_path, entry, cell_library):
    library_name = ctx.attr.library_name or ctx.label.name
    ys_script = ctx.actions.declare_file("%s.ys" % library_name)
    sh_cmd = """cat>{script}<<EOF
# read_verilog -sv {verilog} # if we want to use SV
read_verilog {verilog}
hierarchy -check -top $(cat {entry})
proc; opt;
flatten; opt;
fsm; opt;
memory; opt
techmap; opt
dfflibmap -liberty {cell_library}
abc -liberty {cell_library}
opt_clean -purge
clean
write_verilog {netlist_path}
EOF
    """.format(
        script = ys_script.path,
        verilog = verilog.path,
        entry = entry.path,
        cell_library = cell_library.path,
        netlist_path = netlist_path,
    )

    ctx.actions.run_shell(
        inputs = [entry],
        outputs = [ys_script],
        command = sh_cmd,
    )

    return ys_script

def _generate_netlist(ctx, verilog, entry):
    library_name = ctx.attr.library_name or ctx.label.name
    netlist = ctx.actions.declare_file("%s.netlist.v" % library_name)
    script = _generate_yosys_script(ctx, verilog, netlist.path, entry, ctx.file.cell_library)

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

def _fhe_transpile_impl(ctx):
    ir_file, metadata_file, metadata_entry_file = _build_ir(ctx)

    optimizer = ctx.attr.optimizer
    backend = ctx.attr.backend

    outputs = []
    optimized_files = []
    netlist_file = None
    if optimizer == "yosys":
        optimized_ir_file = _optimize_ir(ctx, ir_file, ".opt.ir", metadata_entry_file)
        optimized_files.append(optimized_ir_file)
        verilog_ir_file = _generate_verilog(ctx, optimized_ir_file, ".v", metadata_entry_file)
        netlist_file, yosys_script_file = _generate_netlist(ctx, verilog_ir_file, metadata_entry_file)
        outputs.extend([verilog_ir_file, netlist_file, yosys_script_file])
    else:
        optimized_files = _optimize_and_booleanify_repeatedly(ctx, ir_file, metadata_entry_file)

    hdrs = _generate_struct_header(ctx, metadata_file, optimizer, backend)

    if optimizer == "yosys":
        ir_input = netlist_file
    else:
        ir_input = _pick_last_bool_file(optimized_files)
    out_cc, out_h = _fhe_transpile_ir(ctx, ir_input, metadata_file, metadata_entry_file, optimizer, backend)
    hdrs.append(out_h)

    outputs = [
        ir_file,
        metadata_file,
        metadata_entry_file,
        out_cc,
    ] + optimized_files + hdrs + outputs

    return [
        DefaultInfo(files = depset(outputs)),
        OutputGroupInfo(
            sources = depset([out_cc]),
            headers = depset(hdrs),
        ),
    ]

def _executable_attr(label):
    """A helper for declaring internal executable dependencies."""
    return attr.label(
        default = Label(label),
        allow_single_file = True,
        executable = True,
        cfg = "exec",
    )

fhe_transpile = rule(
    doc = """
      This rule produces transpiled C++ code that can be included in other
      libraries and binaries.
      """,
    implementation = _fhe_transpile_impl,
    attrs = {
        "src": attr.label(
            doc = "A single C++ source file to transpile.",
            allow_single_file = [".cc"],
        ),
        "hdrs": attr.label_list(
            doc = "Any headers necessary for conversion to XLS IR.",
            allow_files = [".h"],
        ),
        "library_name": attr.string(
            doc = """
            The name used for the output files (<library_name>.cc and <library_name>.h);
            defaults to the name of this target.
            """,
        ),
        "num_opt_passes": attr.int(
            doc = """
            The number of optimization passes to run on XLS IR (default 1).
            Values <= 0 will skip optimization altogether.
            (Only affects the XLS optimizer.)
            """,
            default = 1,
        ),
        "optimizer": attr.string(
            doc = """
            Optimizing/lowering pipeline to use in transpilation. Choices are {xls, yosys}.
            'xls' uses the built-in XLS tools to transform the program into an optimized
            Boolean circuit; 'yosys' uses Yosys to synthesize a circuit that targets the
            given backend.
            """,
            values = [
                "xls",
                "yosys",
            ],
            default = "xls",
        ),
        "backend": attr.string(
            doc = """
            FHE execution backend to target. Choices are {tfhe, palisade, interpreted_tfhe,
            interpreted_palisade, cleartext}. 'cleartext' executes the resulting circuit in
            cleartext, skipping encryption; this has zero security, but is useful for
            debugging the pipeline.
            """,
            values = [
                "tfhe",
                "interpreted_tfhe",
                "palisade",
                "interpreted_palisade",
                "cleartext",
            ],
            default = "tfhe",
        ),
        "cell_library": attr.label(
            doc = "A single cell-definition library in Liberty format.",
            allow_single_file = [".liberty"],
        ),
        "_xlscc": _executable_attr(_XLSCC),
        "_xls_booleanify": _executable_attr(_XLS_BOOLEANIFY),
        "_xls_opt": _executable_attr(_XLS_OPT),
        "_fhe_transpiler": _executable_attr(_FHE_TRANSPILER),
        "_struct_header_generator": _executable_attr(_STRUCT_HEADER_GENERATOR),
        "_get_top_func_from_proto": attr.label(
            default = Label(_GET_TOP_FUNC_FROM_PROTO),
            executable = True,
            cfg = "exec",
        ),
        "_yosys": _executable_attr(_YOSYS),
        "_abc": _executable_attr(_ABC),
        "_xls_codegen": _executable_attr(_XLS_CODEGEN),
    },
)

def fhe_cc_library(
        name,
        src,
        hdrs,
        copts = [],
        num_opt_passes = 1,
        optimizer = "xls",
        backend = "tfhe",
        **kwargs):
    """A rule for building FHE-based cc_libraries.

    Example usage:
        fhe_cc_library(
            name = "secret_computation",
            src = "secret_computation.cc",
            hdrs = ["secret_computation.h"],
            num_opt_passes = 2,
            optimizer = "xls",
            backend = "cleartext",
        )
        cc_binary(
            name = "secret_computation_consumer",
            srcs = ["main.cc"],
            deps = [":secret_computation"],
        )

    To generate just the transpiled sources, you can build the "<TARGET>.transpiled_files"
    subtarget; in the above example, you could run:
        blaze build :secret_computation.transpiled_files

    Args:
      name: The name of the cc_library target.
      src: The transpiler-compatible C++ file that are processed to create the library.
      hdrs: The list of header files required while transpiling the `src`.
      copts: The list of options to the C++ compilation command.
      num_opt_passes: The number of optimization passes to run on XLS IR (default 1).
            Values <= 0 will skip optimization altogether.
            (Only affects the XLS optimizer.)
      optimizer: Defaults to "xls"; optimizing/lowering pipeline to use in transpilation.
            Choices are {xls, yosys}. 'xls' uses the built-in XLS tools to transform the
            program into an optimized Boolean circuit; 'yosys' uses Yosys to synthesize
            a circuit that targets the given backend. (In most cases, Yosys's optimizer
            is more powerful.)
      backend: Defaults to "tfhe"; FHE execution backend to transpile to. Choices are
            {tfhe, interpreted_tfhe, palisade, interpreted_palisade, cleartext}.
            'cleartext' executes the resulting circuit in cleartext, skipping encryption
            entirely; this has zero security, but is useful for debugging the pipeline.
      **kwargs: Keyword arguments to pass through to the cc_library target.
    """
    tags = kwargs.pop("tags", None)

    transpiled_files = "{}.transpiled_files".format(name)
    fhe_transpile(
        name = transpiled_files,
        src = src,
        hdrs = hdrs,
        library_name = name,
        num_opt_passes = num_opt_passes,
        optimizer = optimizer,
        backend = backend,
        tags = tags,
        cell_library = _TFHE_CELLS_LIBERTY,
    )

    transpiled_source = "{}.srcs".format(name)
    native.filegroup(
        name = transpiled_source,
        srcs = [":" + transpiled_files],
        output_group = "sources",
        tags = tags,
    )

    transpiled_headers = "{}.hdrs".format(name)
    native.filegroup(
        name = transpiled_headers,
        srcs = [":" + transpiled_files],
        output_group = "headers",
        tags = tags,
    )

    deps = [
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/types:span",
    ]
    if optimizer == "yosys":
        if backend == "cleartext":
            deps.extend([
                "@com_google_absl//absl/status:statusor",
                "//transpiler:yosys_cleartext_runner",
                "//transpiler/data:boolean_data",
                "@com_google_xls//xls/common/status:status_macros",
            ])
        elif backend == "interpreted_tfhe":
            deps.extend([
                "@com_google_absl//absl/status:statusor",
                "//transpiler:yosys_interpreted_tfhe_runner",
                "//transpiler/data:boolean_data",
                "//transpiler/data:tfhe_data",
                "@tfhe//:libtfhe",
                "@com_google_xls//xls/common/status:status_macros",
            ])
    elif optimizer == "xls":
        if backend == "cleartext":
            deps.extend([
                "//transpiler/data:boolean_data",
            ])
        elif backend == "tfhe":
            deps.extend([
                "@tfhe//:libtfhe",
                "//transpiler/data:boolean_data",
                "//transpiler/data:tfhe_data",
            ])
        elif backend == "interpreted_tfhe":
            deps.extend([
                "@com_google_absl//absl/status:statusor",
                "//transpiler:tfhe_runner",
                "//transpiler/data:boolean_data",
                "//transpiler/data:tfhe_data",
                "@tfhe//:libtfhe",
                "@com_google_xls//xls/common/status:status_macros",
            ])
        elif backend == "palisade":
            deps.extend([
                "//transpiler/data:boolean_data",
                "//transpiler/data:palisade_data",
                "@palisade//:binfhe",
            ])
        elif backend == "interpreted_palisade":
            deps.extend([
                "@com_google_absl//absl/status:statusor",
                "//transpiler:palisade_runner",
                "//transpiler/data:boolean_data",
                "//transpiler/data:palisade_data",
                "@palisade//:binfhe",
                "@com_google_xls//xls/common/status:status_macros",
            ])

    native.cc_library(
        name = name,
        srcs = [":" + transpiled_source],
        hdrs = [":" + transpiled_headers] + hdrs,
        copts = ["-O0"] + copts,
        tags = tags,
        deps = deps,
        **kwargs
    )
