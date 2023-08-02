"""Rules for generating FHE targeting Rust cryptograhpic backends."""

load("@rules_rust//rust:defs.bzl", "rust_library")
load(
    "//transpiler:fhe_common.bzl",
    "BooleanifiedIrInfo",
    "BooleanifiedIrOutputInfo",
    "LUT_CELLS_LIBERTY",
    "TFHERS_CELLS_LIBERTY",
    "executable_attr",
)
load(
    "//transpiler:parsers.bzl",
    "cc_to_xls_ir",
)
load(
    "//transpiler:fhe_xls.bzl",
    "xls_ir_to_verilog",
)
load(
    "//transpiler:fhe_yosys.bzl",
    "VALID_LUT_SIZES",
    "verilog_to_netlist",
)

FHE_OPTIMIZERS = ["yosys"]
_FHE_TRANSPILER = "//transpiler/rust:transpiler"

def _fhe_transpile_ir(ctx, library_name, stem, src, metadata = "", liberty_file = "", parallelism = 0):
    """Transpile XLS IR or a Verilog netlist into Rust source."""
    name = stem if library_name == stem else library_name
    rust_out = ctx.actions.declare_file("%s.rs" % name)

    args = [
        "-ir_path",
        src.path,
        "-rs_out",
        rust_out.path,
        "-parallelism",
        str(parallelism),
    ]
    inputs = [src]

    if liberty_file:
        args += ["-liberty_path", liberty_file.path]
        inputs.append(liberty_file)

    if metadata:
        args += ["-metadata_path", metadata.path]
        inputs.append(metadata)

    ctx.actions.run(
        inputs = inputs,
        outputs = [rust_out],
        executable = ctx.executable._fhe_transpiler,
        arguments = args,
    )
    return rust_out

def _rust_codegen_impl(ctx):
    src = ctx.attr.src

    all_files = src[DefaultInfo].files
    ir = src[BooleanifiedIrOutputInfo].ir.to_list()[0]
    metadata = src[BooleanifiedIrOutputInfo].metadata.to_list()[0]
    library_name = ctx.attr.library_name or src[BooleanifiedIrInfo].library_name
    liberty_file = ctx.file.cell_library
    stem = src[BooleanifiedIrInfo].stem

    rust_out = _fhe_transpile_ir(
        ctx,
        library_name,
        stem,
        ir,
        metadata = metadata,
        liberty_file = liberty_file,
        parallelism = ctx.attr.parallelism,
    )

    input_headers = []
    for hdr in src[BooleanifiedIrOutputInfo].hdrs.to_list():
        input_headers.extend(hdr.files.to_list())
    return [
        DefaultInfo(files = depset([rust_out] + all_files.to_list())),
        OutputGroupInfo(
            sources = depset([rust_out]),
            input_headers = input_headers,
        ),
    ]

_rust_codegen = rule(
    doc = """
    Converts verilog IR into a Rust implementation using an FHE backend.
    """,
    implementation = _rust_codegen_impl,
    attrs = {
        "src": attr.label(
            providers = [BooleanifiedIrOutputInfo, BooleanifiedIrInfo],
            doc = "A single consumable IR source file.",
            mandatory = True,
        ),
        "library_name": attr.string(
            doc = """The name of the output .rs file, overriding the default
            if present.""",
            default = "",
        ),
        "cell_library": attr.label(
            doc = "A single cell-definition library in Liberty format.",
            allow_single_file = [".liberty"],
        ),
        "parallelism": attr.int(
            doc = """
            The number of local devices to parallelize the circuit execution over.
            This tells the scheduler to partition the circuit into batches, so that
            each batch can be executed in parallel over the available devices.

            A value of zero implies unrestricted parallelism to be determined by
            the backend.
            """,
            default = 0,
        ),
        "_fhe_transpiler": executable_attr(_FHE_TRANSPILER),
    },
)

def rust_codegen(name, rust_file_name, src, cell_library = None, parallelism = 0, rustc_flags = [], **kwargs):
    """Given a boolean IR `src`, creates a Jaxite equivalent rust_library.

    Args:
      name: The name of the rust_library target.
      rust_file_name: The stem of the generated rust file.
      src: The boolean IR files to transpile.
      cell_library: The liberty file to use for verilog cell definitions.
      parallelism: The number of devices to parallelize circuit execution over.
      **kwargs: rust_library keyword arguments passed to the resulting library target.
    """
    tags = kwargs.pop("tags", None)
    transpiled_files = "{}.transpiled_files".format(name)

    _rust_codegen(
        name = transpiled_files,
        src = src,
        library_name = rust_file_name,
        cell_library = cell_library,
        parallelism = parallelism,
    )

    transpiled_source = "{}.srcs".format(name)
    native.filegroup(
        name = transpiled_source,
        srcs = [":" + transpiled_files],
        output_group = "sources",
        tags = tags,
    )

    rust_library(
        name = name,
        srcs = [":" + transpiled_source],
        tags = tags,
        deps = [
            "@crate_index//:rayon",
            "@crate_index//:tfhe",
        ],
        rustc_flags = rustc_flags,
        **kwargs
    )

def fhe_rust_library(
        name,
        src,
        hdrs,
        optimizer = "yosys",
        generated_rust_file_name = None,
        lut_size = 3,
        parallelism = 0,
        **kwargs):
    """A rule for building FHE-based rust_libraries.

    Example usage:
      fhe_rust_library(
        name = "secret computation",
        src = "secret_computation.cc",
        hdrs = ["secret_computation.h"],
      )
      rust_binary(
        name = "secret_computation_consumer",
        srcs = ["main.rs"],
        deps = [":secret_computation"],
      )

    Args:
      name: The name of the rust_library target.
      src: The transpiler-compatible C++ file that is processed to create the library.
      hdrs: The list of header files required while transpiling the `src`.
      optimizer: The middleware to use to optimize the circuit.
      generated_rust_file_name: The name of the generated .rs file, defaulting to `name`
            if omitted.
      lut_size: The bit-width of look-up tables that should be used. If set to
            0, the output circuit will use a standard set of named 2-bit boolean
            gates. If set to 2 or higher, it will use lut_size-bit lookup tables,
            which generally makes the circuit smaller. Set to 3 by default. Only
            supported for the `"yosys"` optimizer.
      parallelism: The amount of device parallelism available to the backend
            platform. E.g., for a TPU pod with 8 local devices, this would be
            set to 8, and the resulting code would partition its circuit into
            batches of size 8, executing the gates in parallel in each batch.
            If zero (the default), gates are run in serial.
      **kwargs: Keyword arguments to pass through to the rust_library target.
    """
    if optimizer not in FHE_OPTIMIZERS:
        fail("Invalid optimizer: ", optimizer)

    valid_lut_sizes = VALID_LUT_SIZES.get(optimizer)
    if not valid_lut_sizes:
        fail("No valid lut sizes configured for ", optimizer, ". Please report a bug.")

    if lut_size > 0 and lut_size != 3:
        fail("Only lut_size=3 is supported")

    rust_file_name = generated_rust_file_name or name
    transpiled_xlscc_files = "{}.cc_to_xls_ir".format(name)
    cc_to_xls_ir(
        name = transpiled_xlscc_files,
        library_name = name,
        src = src,
        hdrs = hdrs,
    )

    verilog = "{}.verilog".format(name)
    xls_ir_to_verilog(
        name = verilog,
        src = ":" + transpiled_xlscc_files,
    )
    netlist = "{}.netlist".format(name)

    cell_library = TFHERS_CELLS_LIBERTY
    rustc_flags = []
    if lut_size > 0:
        cell_library = LUT_CELLS_LIBERTY
        rustc_flags = ["--cfg", "lut"]

    verilog_to_netlist(
        name = netlist,
        src = ":" + verilog,
        encryption = "tfhe-rs",
        lut_size = lut_size,
        cell_library = cell_library,
    )
    rust_codegen(
        name = name,
        rust_file_name = rust_file_name,
        cell_library = cell_library,
        src = ":" + netlist,
        parallelism = parallelism,
        rustc_flags = rustc_flags,
        **kwargs
    )
