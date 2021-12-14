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

def _optimize_ir(ctx, src, extension, entry):
    """Optimize XLS IR."""
    return _run(ctx, [src, entry], extension, ctx.executable._xls_opt, [src.path], entry)

def _booleanify_ir(ctx, src, extension, entry):
    """Optimize XLS IR."""
    return _run(ctx, [src, entry], extension, ctx.executable._xls_booleanify, ["--ir_path", src.path], entry)

def _fhe_transpile_ir(ctx, src, metadata, entry):
    """Transpile XLS IR into C++ source."""
    library_name = ctx.attr.library_name or ctx.label.name
    out_ir = ctx.actions.declare_file("%s.bool.ir" % library_name)
    out_cc = ctx.actions.declare_file("%s.cc" % library_name)
    out_h = ctx.actions.declare_file("%s.h" % library_name)

    args = [
        "-ir_path",
        src.path,
        "-metadata_path",
        metadata.path,
        "-output_ir_path",
        out_ir.path,
        "-cc_path",
        out_cc.path,
        "-header_path",
        out_h.path,
        "-opt_passes",
        str(ctx.attr.num_opt_passes),
        "-booleanify_main_path",
        ctx.executable._xls_booleanify.path,
        "-opt_main_path",
        ctx.executable._xls_opt.path,
        "-transpiler_type",
        ctx.attr.transpiler_type,
    ]
    ctx.actions.run(
        inputs = [src, metadata],
        outputs = [out_ir, out_cc, out_h],
        executable = ctx.executable._fhe_transpiler,
        arguments = args,
        tools = [
            ctx.executable._xls_booleanify,
            ctx.executable._xls_opt,
        ],
    )
    return [out_ir, out_cc, out_h]

def _generate_struct_header(ctx, metadata):
    """Transpile XLS IR into C++ source."""
    library_name = ctx.attr.library_name or ctx.label.name
    struct_h = ctx.actions.declare_file("%s.types.h" % library_name)
    args = [
        "-metadata_path",
        metadata.path,
        "-original_headers",
        ",".join([hdr.path for hdr in ctx.files.hdrs]),
        "-output_path",
        struct_h.path,
    ]
    ctx.actions.run(
        inputs = [metadata],
        outputs = [struct_h],
        executable = ctx.executable._struct_header_generator,
        arguments = args,
    )
    return struct_h

def _fhe_transpile_impl(ctx):
    ir_file, metadata_file, metadata_entry_file = _build_ir(ctx)
    optimized_ir_file = _optimize_ir(ctx, ir_file, ".opt.ir", metadata_entry_file)
    booleanified_ir_file = _booleanify_ir(ctx, optimized_ir_file, ".opt.bool.ir", metadata_entry_file)

    hdrs = []
    if ctx.attr.transpiler_type != "bool":
        hdrs.append(_generate_struct_header(ctx, metadata_file))

    bool_ir, out_cc, out_h = _fhe_transpile_ir(ctx, ir_file, metadata_file, metadata_entry_file)
    hdrs.append(out_h)
    return [
        DefaultInfo(files = depset([ir_file, optimized_ir_file, booleanified_ir_file, metadata_file, metadata_entry_file, bool_ir, out_cc] + hdrs)),
        OutputGroupInfo(
            sources = depset([out_cc]),
            headers = depset(hdrs),
            bool_ir = depset([bool_ir]),
            metadata = depset([metadata_file]),
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
            """,
            default = 1,
        ),
        "transpiler_type": attr.string(
            doc = """
            Type of FHE library to transpile to. Choices are {tfhe, interpreted_tfhe,
            bool}. 'bool' doesn't depend on any FHE libraries.
            """,
            values = ["tfhe", "interpreted_tfhe", "bool"],
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
    },
)

def fhe_cc_library(
        name,
        src,
        hdrs,
        copts = [],
        num_opt_passes = 1,
        transpiler_type = "tfhe",
        **kwargs):
    """A rule for building FHE-based cc_libraries.

    Example usage:
        fhe_cc_library(
            name = "secret_computation",
            src = "secret_computation.cc",
            hdrs = ["secret_computation.h"],
            num_opt_passes = 2,
            transpiler_type = "bool",
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
      num_opt_passes: The number of optimization passes to run on XLS IR (default 1).
            Values <= 0 will skip optimization altogether.
      transpiler_type: Defaults to "tfhe"; Type of FHE library to transpile to. Choices are
            {tfhe, interpreted_tfhe, bool}. 'bool' does Boolean operations on plaintext, and
            doesn't depend on any FHE libraries; mostly useful for debugging.
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
        transpiler_type = transpiler_type,
        tags = tags,
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

    transpiled_ir = "{}.bool_ir".format(name)
    native.filegroup(
        name = transpiled_ir,
        srcs = [":" + transpiled_files],
        output_group = "bool_ir",
        tags = tags,
    )

    transpiled_metadata = "{}.metadata".format(name)
    native.filegroup(
        name = transpiled_metadata,
        srcs = [":" + transpiled_files],
        output_group = "metadata",
        tags = tags,
    )

    deps = ["@com_google_absl//absl/status"]
    if transpiler_type == "bool":
        deps.append("@com_google_absl//absl/types:span")
    elif transpiler_type == "tfhe":
        deps.extend([
            "@tfhe//:libtfhe",
            "//transpiler/data:fhe_data",
        ])
    elif transpiler_type == "interpreted_tfhe":
        deps.extend([
            "@com_google_absl//absl/status:statusor",
            "//transpiler:tfhe_runner",
            "//transpiler/data:fhe_data",
            "@tfhe//:libtfhe",
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
