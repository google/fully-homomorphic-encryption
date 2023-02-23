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

load(
    "//transpiler/data:primitives.bzl",
    "FHE_PRIMITIVES",
)
load(
    "//transpiler:fhe_common.bzl",
    "BooleanifiedIrInfo",
    "BooleanifiedIrOutputInfo",
    "FHE_ENCRYPTION_SCHEMES",
    "executable_attr",
)
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
    "//transpiler:fhe_yosys.bzl",
    "NetlistEncryptionInfo",
    "verilog_to_netlist",
)
load(
    "//transpiler:fhe_structs.bzl",
    "TranspiledStructsOutputInfo",
    "xls_cc_transpiled_structs",
)

_FHE_TRANSPILER = "//transpiler"

FHE_OPTIMIZERS = [
    "xls",
    "yosys",
]

def _fhe_transpile_ir(ctx, library_name, stem, src, metadata, optimizer, encryption, encryption_specific_transpiled_structs_header, interpreter, skip_scheme_data_deps, unwrap):
    """Transpile XLS IR into C++ source."""

    if library_name == stem:
        name = stem + ("_yosys" if optimizer == "yosys" else "") + ("_interpreted" if interpreter else "") + "_" + encryption
    else:
        name = library_name
    out_cc = ctx.actions.declare_file("%s.cc" % name)
    out_h = ctx.actions.declare_file("%s.h" % name)

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
        "-encryption",
        encryption,
        "-encryption_specific_transpiled_structs_header_path",
        encryption_specific_transpiled_structs_header.short_path,
    ]
    if interpreter:
        args.append("-interpreter")
    if skip_scheme_data_deps:
        args.append("-skip_scheme_data_deps")
    if len(unwrap):
        args += [
            "-unwrap",
            ",".join(unwrap),
        ]

    ctx.actions.run(
        inputs = [src, metadata, encryption_specific_transpiled_structs_header],
        outputs = [out_cc, out_h],
        executable = ctx.executable._fhe_transpiler,
        arguments = args,
    )
    return [out_cc, out_h]

def _fhe_transpile_netlist(ctx, library_name, stem, src, metadata, optimizer, encryption, encryption_specific_transpiled_structs_header, interpreter, unwrap):
    """Transpile XLS IR into C++ source."""

    if library_name == stem:
        name = stem + ("_yosys" if optimizer == "yosys" else "") + ("_interpreted" if interpreter else "") + "_" + encryption
    else:
        name = library_name
    out_cc = ctx.actions.declare_file("%s.cc" % name)
    out_h = ctx.actions.declare_file("%s.h" % name)

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
        "-encryption",
        encryption,
        "-encryption_specific_transpiled_structs_header_path",
        encryption_specific_transpiled_structs_header.short_path,
    ]
    if interpreter:
        args.append("-interpreter")

    args += ["-liberty_path", ctx.file.cell_library.path]

    if len(unwrap):
        args += [
            "-unwrap",
            ",".join(unwrap),
        ]

    ctx.actions.run(
        inputs = [src, metadata, encryption_specific_transpiled_structs_header, ctx.file.cell_library],
        outputs = [out_cc, out_h],
        executable = ctx.executable._fhe_transpiler,
        arguments = args,
    )
    return [out_cc, out_h]

FheIrLibraryOutputInfo = provider(
    """Provides the generates headers by the FHE transpiler, as well as the
       passed-along headers directly provided by the user.""",
    fields = {
        "hdrs": "Input C++ headers",
    },
)

def _cc_fhe_ir_library_impl(ctx):
    interpreter = ctx.attr.interpreter
    encryption = ctx.attr.encryption
    src = ctx.attr.src
    transpiled_structs = ctx.attr.transpiled_structs
    skip_scheme_data_deps = ctx.attr.skip_scheme_data_deps

    all_files = src[DefaultInfo].files
    ir = src[BooleanifiedIrOutputInfo].ir.to_list()[0]
    metadata = src[BooleanifiedIrOutputInfo].metadata.to_list()[0]
    library_name = src[BooleanifiedIrInfo].library_name
    stem = src[BooleanifiedIrInfo].stem
    optimizer = src[BooleanifiedIrInfo].optimizer
    generic_transpiled_structs_header = transpiled_structs[TranspiledStructsOutputInfo].generic_struct_header.to_list()[0]
    encryption_specific_transpiled_structs_header = transpiled_structs[TranspiledStructsOutputInfo].encryption_specific_transpiled_structs_header.to_list()[0]

    # Netlists need to be generated with knowledge of the encryption scheme,
    # since the scheme affects the choice of cell library.  Make sure that the
    # netlist was generated to target the correct encryption scheme.
    if NetlistEncryptionInfo in src:
        src_encryption = src[NetlistEncryptionInfo].encryption
        if encryption != src_encryption:
            fail("Netlist was not generated for the same encryption scheme! Expecting {} but src has {}.".format(encryption, src_encryption))

    if optimizer == "yosys":
        out_cc, out_h = _fhe_transpile_netlist(
            ctx,
            library_name,
            stem,
            ir,
            metadata,
            optimizer,
            encryption,
            encryption_specific_transpiled_structs_header,
            interpreter,
            FHE_PRIMITIVES,
        )
    else:
        out_cc, out_h = _fhe_transpile_ir(
            ctx,
            library_name,
            stem,
            ir,
            metadata,
            optimizer,
            encryption,
            encryption_specific_transpiled_structs_header,
            interpreter,
            skip_scheme_data_deps,
            FHE_PRIMITIVES,
        )

    input_headers = []
    for hdr in src[BooleanifiedIrOutputInfo].hdrs.to_list():
        input_headers.extend(hdr.files.to_list())

    output_hdrs = [
        out_h,
        generic_transpiled_structs_header,
        encryption_specific_transpiled_structs_header,
    ]
    outputs = [out_cc] + output_hdrs
    return [
        DefaultInfo(files = depset(outputs + all_files.to_list())),
        OutputGroupInfo(
            sources = depset([out_cc]),
            headers = depset(output_hdrs),
            input_headers = input_headers,
        ),
    ]

_cc_fhe_bool_ir_library = rule(
    doc = """
      This rule produces transpiled C++ code that can be included in other
      libraries and binaries.
      """,
    implementation = _cc_fhe_ir_library_impl,
    attrs = {
        "src": attr.label(
            providers = [BooleanifiedIrOutputInfo, BooleanifiedIrInfo],
            doc = "A single consumable IR source file.",
            mandatory = True,
        ),
        "transpiled_structs": attr.label(
            providers = [TranspiledStructsOutputInfo],
            doc = "Target with generic and scheme-specific encodings of XLScc data types",
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
        "interpreter": attr.bool(
            doc = """
            Controls whether the resulting program executes directly (single-threaded C++),
            or invokes a multi-threaded interpreter.
            """,
            default = False,
        ),
        "skip_scheme_data_deps": attr.bool(
            doc = """
            When set to True, it causes the transpiler to not emit depednencies
            for tfhe_data.h, openfhe_data.h, and cleartext_data.h.  This is used
            to avoid circular dependencies when generating C++ libraries for
            the numeric primitives.
            """,
            default = False,
        ),
        "_fhe_transpiler": executable_attr(_FHE_TRANSPILER),
    },
)

_cc_fhe_netlist_library = rule(
    doc = """
      This rule produces transpiled C++ code that can be included in other
      libraries and binaries.
      """,
    implementation = _cc_fhe_ir_library_impl,
    attrs = {
        "src": attr.label(
            providers = [BooleanifiedIrOutputInfo, BooleanifiedIrInfo, NetlistEncryptionInfo],
            doc = "A single consumable IR source file.",
            mandatory = True,
        ),
        "transpiled_structs": attr.label(
            providers = [TranspiledStructsOutputInfo],
            doc = "Target with scheme-specific encodings of XLScc data types",
            mandatory = False,
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
        "interpreter": attr.bool(
            doc = """
            Controls whether the resulting program executes directly (single-threaded C++),
            or invokes a multi-threaded interpreter.
            """,
            default = False,
        ),
        "skip_scheme_data_deps": attr.bool(
            doc = """
            When set to True, it causes the transpiler to not emit depednencies
            for tfhe_data.h, openfhe_data.h, and cleartext_data.h.  This is used
            to avoid circular dependencies when generating C++ libraries for
            the numeric primitives.

            Always set to False when generating netlist libraries.
            """,
            default = False,
        ),
        "cell_library": attr.label(
            doc = "A single cell-definition library in Liberty format.",
            allow_single_file = [".liberty"],
        ),
        "_fhe_transpiler": executable_attr(_FHE_TRANSPILER),
    },
)

def _cc_fhe_common_library(name, optimizer, src, transpiled_structs, encryption, interpreter, hdrs = [], copts = [], skip_scheme_data_deps = False, **kwargs):
    tags = kwargs.pop("tags", None)

    transpiled_files = "{}.transpiled_files".format(name)

    if encryption in FHE_ENCRYPTION_SCHEMES:
        if optimizer not in FHE_OPTIMIZERS:
            fail("Invalid optimizer:", optimizer)
        if optimizer == "xls":
            _cc_fhe_bool_ir_library(
                name = transpiled_files,
                src = src,
                transpiled_structs = transpiled_structs,
                encryption = encryption,
                interpreter = interpreter,
                skip_scheme_data_deps = skip_scheme_data_deps,
            )
        else:  # optimizer == "yosys":
            _cc_fhe_netlist_library(
                name = transpiled_files,
                src = src,
                transpiled_structs = transpiled_structs,
                encryption = encryption,
                interpreter = interpreter,
                cell_library = FHE_ENCRYPTION_SCHEMES[encryption],
                skip_scheme_data_deps = False,
            )
    else:
        fail("Invalid encryption value:", encryption)

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

    input_headers = "{}.input.hdrs".format(name)
    native.filegroup(
        name = input_headers,
        srcs = [":" + transpiled_files],
        output_group = "input_headers",
        tags = tags,
    )

    deps = [
        "@com_google_xls//xls/common/logging",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/types:span",
        "//transpiler:common_runner",
        "//transpiler/data:cleartext_value",
        "//transpiler/data:generic_value",
    ]

    if encryption == "cleartext":
        pass
    elif encryption == "tfhe":
        deps.extend([
            "@tfhe//:libtfhe",
            "//transpiler/data:tfhe_value",
        ])
    elif encryption == "openfhe":
        deps.extend([
            "@openfhe//:binfhe",
            "//transpiler/data:openfhe_value",
        ])

    if not skip_scheme_data_deps:
        if optimizer == "xls":
            if encryption == "cleartext":
                if interpreter:
                    fail("No XLS interpreter for cleartext is currently implemented.")
                deps.extend([
                    "//transpiler/data:cleartext_data",
                ])
            elif encryption == "tfhe":
                deps.extend([
                    "//transpiler/data:cleartext_data",
                    "//transpiler/data:tfhe_data",
                ])
                if interpreter:
                    deps.extend([
                        "@com_google_absl//absl/status:statusor",
                        "//transpiler:tfhe_runner",
                        "@com_google_xls//xls/common/status:status_macros",
                    ])
            elif encryption == "openfhe":
                deps.extend([
                    "//transpiler/data:cleartext_data",
                    "//transpiler/data:openfhe_data",
                ])
                if interpreter:
                    deps.extend([
                        "@com_google_absl//absl/status:statusor",
                        "//transpiler:openfhe_runner",
                        "@com_google_xls//xls/common/status:status_macros",
                    ])
        else:
            if not interpreter:
                fail("The Yosys pipeline only implements interpreter execution.")
            if encryption == "cleartext":
                deps.extend([
                    "@com_google_absl//absl/status:statusor",
                    "//transpiler:yosys_cleartext_runner",
                    "//transpiler/data:cleartext_data",
                    "@com_google_xls//xls/common/status:status_macros",
                ])
            elif encryption == "tfhe":
                deps.extend([
                    "@com_google_absl//absl/status:statusor",
                    "//transpiler:yosys_tfhe_runner",
                    "//transpiler/data:cleartext_data",
                    "//transpiler/data:tfhe_data",
                    "@com_google_xls//xls/common/status:status_macros",
                ])
            elif encryption == "openfhe":
                deps.extend([
                    "@com_google_absl//absl/status:statusor",
                    "//transpiler:yosys_openfhe_runner",
                    "//transpiler/data:cleartext_data",
                    "//transpiler/data:openfhe_data",
                    "@com_google_xls//xls/common/status:status_macros",
                ])

    native.cc_library(
        name = name,
        srcs = [":" + transpiled_source],
        hdrs = [":" + transpiled_headers, ":" + input_headers] + hdrs,
        copts = ["-O0"] + copts,
        tags = tags,
        deps = deps,
        **kwargs
    )

def cc_fhe_bool_ir_library(name, src, transpiled_structs, encryption, interpreter, copts = [], skip_scheme_data_deps = False, **kwargs):
    _cc_fhe_common_library(
        name = name,
        optimizer = "xls",
        src = src,
        transpiled_structs = transpiled_structs,
        encryption = encryption,
        interpreter = interpreter,
        copts = copts,
        skip_scheme_data_deps = skip_scheme_data_deps,
        **kwargs
    )

def cc_fhe_netlist_library(name, src, encryption, transpiled_structs, interpreter, copts = [], **kwargs):
    _cc_fhe_common_library(
        name = name,
        optimizer = "yosys",
        src = src,
        transpiled_structs = transpiled_structs,
        encryption = encryption,
        interpreter = interpreter,
        copts = copts,
        **kwargs
    )

def fhe_cc_library(
        name,
        src,
        hdrs,
        copts = [],
        num_opt_passes = 1,
        encryption = "tfhe",
        optimizer = "xls",
        interpreter = False,
        library_name = None,
        loop_unroll_rlimit = 10000,
        **kwargs):
    """A rule for building FHE-based cc_libraries.

    Example usage:
        fhe_cc_library(
            name = "secret_computation",
            src = "secret_computation.cc",
            hdrs = ["secret_computation.h"],
            num_opt_passes = 2,
            encryption = "cleartext",
            optimizer = "xls",
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
      encryption: Defaults to "tfhe"; FHE encryption scheme used by the resulting program.
            Choices are {tfhe, openfhe, cleartext}. 'cleartext' means the program runs in
            cleartext, skipping encryption; this has zero security, but is useful for
            debugging.
      optimizer: Defaults to "xls"; optimizing/lowering pipeline to use in transpilation.
            Choices are {xls, yosys}. 'xls' uses the built-in XLS tools to transform the
            program into an optimized Boolean circuit; 'yosys' uses Yosys to synthesize
            a circuit that targets the given encryption. (In most cases, Yosys's optimizer
            is more powerful.)
      interpreter: Defaults to False; controls whether the resulting program executes
            directly (single-threaded C++), or invokes a multi-threaded interpreter.
      library_name: The name stem to use for the generated .h and .cc files,
            defaulting to `name` if not provided.
      loop_unroll_rlimit: The computation limit to pass to xlscc, to instruct its Z3
            solver on how much effort to spend trying to prove that loops unwrap.
            See fhe_xls.bzl::cc_to_xls_ir.z3_rlimit for more details.
      **kwargs: Keyword arguments to pass through to the cc_library target.
    """
    transpiled_xlscc_files = "{}.cc_to_xls_ir".format(name)
    library_name = library_name or name
    cc_to_xls_ir(
        name = transpiled_xlscc_files,
        library_name = library_name,
        src = src,
        hdrs = hdrs,
        z3_rlimit = loop_unroll_rlimit,
        defines = kwargs.get("defines", None),
    )

    if optimizer not in FHE_OPTIMIZERS:
        fail("Invalid optimizer:", optimizer)

    # Optimize the generated IR
    optimized_intermediate_files = "{}.optimized_bool_ir".format(name)
    netlist = "{}.netlist".format(name)
    verilog = "{}.verilog".format(name)
    if optimizer == "xls":
        xls_ir_to_bool_ir(
            name = optimized_intermediate_files,
            src = ":" + transpiled_xlscc_files,
            num_opt_passes = num_opt_passes,
        )
    else:
        xls_ir_to_verilog(
            name = verilog,
            src = ":" + transpiled_xlscc_files,
        )
        verilog_to_netlist(
            name = netlist,
            src = ":" + verilog,
            encryption = encryption,
        )

    # Generate libraries for structs
    transpiled_structs_headers = "{}.xls_cc_transpiled_structs".format(name)
    xls_cc_transpiled_structs(
        name = transpiled_structs_headers,
        src = ":" + transpiled_xlscc_files,
        encryption = encryption,
    )

    # Transpile the optimized ir
    if optimizer == "xls":
        cc_fhe_bool_ir_library(
            name = name,
            src = ":" + optimized_intermediate_files,
            encryption = encryption,
            interpreter = interpreter,
            transpiled_structs = ":" + transpiled_structs_headers,
            copts = copts,
            **kwargs
        )
    else:
        cc_fhe_netlist_library(
            name = name,
            src = ":" + netlist,
            encryption = encryption,
            interpreter = interpreter,
            transpiled_structs = ":" + transpiled_structs_headers,
            copts = copts,
            **kwargs
        )
