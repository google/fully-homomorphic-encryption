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

"""Common routines for working with XLS in bazel files."""

load(
    "//transpiler:fhe_common.bzl",
    "BooleanifiedIrInfo",
    "BooleanifiedIrOutputInfo",
    "VerilogInfo",
    "VerilogOutputInfo",
    "executable_attr",
    "run_with_stem",
)
load(
    "//transpiler:parsers.bzl",
    "XlsCcOutputInfo",
)

_XLS_BOOLEANIFY = "@com_google_xls//xls/dev_tools:booleanify_main"
_XLS_CODEGEN = "@com_google_xls//xls/tools:codegen_main"
_XLS_OPT = "@com_google_xls//xls/tools:opt_main"

def _pick_last_bool_file(optimized_files):
    """Pick the last booleanifed IR file from a list of file produced by _optimize_and_booleanify_repeatedly().

    The last %.*.bool.ir file may or may not be the smallest one.  For some IR
    inputs, an additional optimization/booleanification pass results in a
    larger file.  This is why we have num_opt_passes.
    """

    # structure is [%.opt.ir, %.opt.bool.ir, %.opt.bool.opt.ir,
    # %.opt.bool.opt.bool.ir, ...], so every other file is the result of an
    # optimization + booleanification pass.
    return optimized_files[-1]

def optimize_ir(ctx, stem, src, extension, entry, options = []):
    """Optimize XLS IR."""
    return run_with_stem(ctx, stem, [src, entry], extension, ctx.executable._xls_opt, [src.path] + options, entry)

def _booleanify_ir(ctx, stem, src, extension, entry):
    """Booleanify XLS IR."""
    return run_with_stem(ctx, stem, [src, entry], extension, ctx.executable._xls_booleanify, ["--ir_path", src.path], entry)

def _optimize_and_booleanify_repeatedly(ctx, stem, ir_file, entry):
    """Runs several passes of optimization followed by booleanification.

    Returns [%.opt.ir, %.opt.bool.ir, %.opt.bool.opt.ir, %.opt.bool.opt.bool.ir, ...]
    """
    results = [ir_file]
    suffix = ""

    # With zero optimization passes, we still want to run the optimizer with an
    # inlining pass, as the booleanifier expects a single function.
    if ctx.attr.num_opt_passes == 0:
        suffix += ".opt"
        results.append(optimize_ir(ctx, stem, results[-1], suffix + ".ir", entry, ["--passes=inlining"]))
        suffix += ".bool"
        results.append(_booleanify_ir(ctx, stem, results[-1], suffix + ".ir", entry))
    else:
        for _ in range(ctx.attr.num_opt_passes):
            suffix += ".opt"
            results.append(optimize_ir(ctx, stem, results[-1], suffix + ".ir", entry))
            suffix += ".bool"
            results.append(_booleanify_ir(ctx, stem, results[-1], suffix + ".ir", entry))
    return results[1:]

def _xls_ir_to_bool_ir_impl(ctx):
    src = ctx.attr.src
    ir_input = src[XlsCcOutputInfo].ir.to_list()[0]
    metadata_file = src[XlsCcOutputInfo].metadata.to_list()[0]
    metadata_entry_file = src[XlsCcOutputInfo].metadata_entry.to_list()[0]
    library_name = src[XlsCcOutputInfo].library_name
    stem = src[XlsCcOutputInfo].stem

    optimized_files = _optimize_and_booleanify_repeatedly(ctx, library_name, ir_input, metadata_entry_file)
    ir_output = _pick_last_bool_file(optimized_files)

    return [
        DefaultInfo(files = depset(optimized_files + [ir_output] + src[DefaultInfo].files.to_list())),
        BooleanifiedIrOutputInfo(
            ir = depset([ir_output]),
            metadata = depset([metadata_file]),
            hdrs = depset(src[XlsCcOutputInfo].hdrs),
        ),
        BooleanifiedIrInfo(
            library_name = library_name,
            stem = stem,
            optimizer = "xls",
        ),
    ]

xls_ir_to_bool_ir = rule(
    doc = """
      This rule takes XLS IR output by XLScc and goes through zero or more
      phases of booleanification and optimization.  The output is an optimized
      booleanified XLS IR file.
      """,
    implementation = _xls_ir_to_bool_ir_impl,
    attrs = {
        "src": attr.label(
            providers = [XlsCcOutputInfo],
            doc = "A single XLS IR source file (emitted by XLScc).",
            mandatory = True,
        ),
        "num_opt_passes": attr.int(
            doc = """
            The number of optimization passes to run on XLS IR (default 1).
            Values <= 0 will skip optimization altogether.
            """,
            default = 1,
        ),
        "_xls_booleanify": executable_attr(_XLS_BOOLEANIFY),
        "_xls_opt": executable_attr(_XLS_OPT),
    },
)

def _generate_verilog(ctx, stem, src, extension, entry):
    """Convert optimized XLS IR to Verilog."""
    return run_with_stem(
        ctx,
        stem,
        [src, entry],
        extension,
        ctx.executable._xls_codegen,
        [
            src.path,
            "--delay_model=unit",
            "--generator=combinational",
            "--use_system_verilog=false",  # edit the YS script if this changes
        ],
        entry,
    )

def _xls_ir_to_verilog_impl(ctx):
    src = ctx.attr.src
    ir_input = src[XlsCcOutputInfo].ir.to_list()[0]
    metadata_file = src[XlsCcOutputInfo].metadata.to_list()[0]
    metadata_entry_file = src[XlsCcOutputInfo].metadata_entry.to_list()[0]
    library_name = src[XlsCcOutputInfo].library_name
    stem = src[XlsCcOutputInfo].stem

    optimized_ir_file = optimize_ir(ctx, library_name, ir_input, ".opt.ir", metadata_entry_file)
    verilog_ir_file = _generate_verilog(ctx, library_name, optimized_ir_file, ".v", metadata_entry_file)

    return [
        DefaultInfo(files = depset([optimized_ir_file, verilog_ir_file] + src[DefaultInfo].files.to_list())),
        VerilogOutputInfo(
            verilog_ir_file = depset([verilog_ir_file]),
            metadata = depset([metadata_file]),
            metadata_entry = depset([metadata_entry_file]),
            hdrs = depset(src[XlsCcOutputInfo].hdrs),
        ),
        VerilogInfo(
            library_name = library_name,
            stem = stem,
        ),
    ]

xls_ir_to_verilog = rule(
    doc = """
      This rule takes XLS IR output by XLScc and emits synthesizeable
      combinational Verilog."
      """,
    implementation = _xls_ir_to_verilog_impl,
    attrs = {
        "src": attr.label(
            providers = [XlsCcOutputInfo],
            doc = "A single XLS IR source file (emitted by XLScc).",
            mandatory = True,
        ),
        "_xls_opt": executable_attr(_XLS_OPT),
        "_xls_codegen": executable_attr(_XLS_CODEGEN),
    },
)
