"""Rules for generating FHE targeting Python cryptograhpic backends."""

load("@rules_python//python:defs.bzl", "py_library")
load(
    "//transpiler:fhe_common.bzl",
    "BooleanifiedIrInfo",
    "BooleanifiedIrOutputInfo",
    "LUT_CELLS_LIBERTY",
    "TFHE_CELLS_LIBERTY",
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
    "VALID_LUT_SIZES",
    "verilog_to_netlist",
)

FHE_OPTIMIZERS = ["xls", "yosys"]
_FHE_TRANSPILER = "//transpiler/jaxite:transpiler"

def _fhe_transpile_ir(ctx, library_name, stem, src, metadata = "", liberty_file = "", parallelism = 0):
    """Transpile XLS IR or a Verilog netlist into Python source."""
    name = stem if library_name == stem else library_name
    py_out = ctx.actions.declare_file("%s.py" % name)

    args = [
        "-ir_path",
        src.path,
        "-py_out",
        py_out.path,
        "-optimizer",
        "yosys" if liberty_file else "xls",
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
        outputs = [py_out],
        executable = ctx.executable._fhe_transpiler,
        arguments = args,
    )
    return py_out

def _py_fhe_bool_ir_library_impl(ctx):
    src = ctx.attr.src

    all_files = src[DefaultInfo].files
    ir = src[BooleanifiedIrOutputInfo].ir.to_list()[0]
    metadata = src[BooleanifiedIrOutputInfo].metadata.to_list()[0]
    library_name = ctx.attr.library_name or src[BooleanifiedIrInfo].library_name
    liberty_file = ctx.file.cell_library
    stem = src[BooleanifiedIrInfo].stem

    py_out = _fhe_transpile_ir(
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
        DefaultInfo(files = depset([py_out] + all_files.to_list())),
        OutputGroupInfo(
            sources = depset([py_out]),
            input_headers = input_headers,
        ),
    ]

_py_fhe_bool_ir_library = rule(
    doc = """
    This rule produces transpiled Python code that can be included in other libraries and binaries.
    """,
    implementation = _py_fhe_bool_ir_library_impl,
    attrs = {
        "src": attr.label(
            providers = [BooleanifiedIrOutputInfo, BooleanifiedIrInfo],
            doc = "A single consumable IR source file.",
            mandatory = True,
        ),
        "library_name": attr.string(
            doc = """The name of the output .py file, overriding the default
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

            A value of zero means that the circuit is scheduled to run in serial,
            i.e., no parallelism.
            """,
            default = 0,
        ),
        "_fhe_transpiler": executable_attr(_FHE_TRANSPILER),
    },
)

def py_fhe_bool_ir_library(name, py_file_name, src, cell_library = None, parallelism = 0, **kwargs):
    """Given a boolean IR `src`, creates a Jaxite equivalent py_library.

    Args:
      name: The name of the py_library target.
      py_file_name: The stem of the generated python file.
      src: The boolean IR files to transpile.
      cell_library: The liberty file to use for verilog cell definitions.
      parallelism: The number of devices to parallelize circuit execution over.
      **kwargs: py_library keyword arguments passed to the resulting library target.
    """
    tags = kwargs.pop("tags", None)
    transpiled_files = "{}.transpiled_files".format(name)

    _py_fhe_bool_ir_library(
        name = transpiled_files,
        src = src,
        library_name = py_file_name,
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

    py_library(
        name = name,
        srcs = [":" + transpiled_source],
        tags = tags,
        deps = [
            "@transpiler_pip_deps//pypi__jaxite",
        ],
        **kwargs
    )

def fhe_py_library(
        name,
        src,
        hdrs,
        optimizer = "yosys",
        num_opt_passes = 1,
        generated_py_file_name = None,
        lut_size = 3,
        parallelism = 0,
        **kwargs):
    """A rule for building FHE-based py_libraries.

    Example usage:
      fhe_py_library(
        name = "secret computation",
        src = "secret_computation.cc",
        hdrs = ["secret_computation.h"],
        num_opt_passes = 2,
      )
      py_binary(
        name = "secret_computation_consumer",
        srcs = ["main.py"],
        deps = [":secret_computation"],
      )
    To generate just the transpiled sources, you can build the "<TARGET>.transpiled_files" subtarget.
    In the above example, you could run:
      blaze build :secret_computation.transpiled_files

    Args:
      name: The name of the py_library target.
      src: The transpiler-compatible C++ file that is processed to create the library.
      hdrs: The list of header files required while transpiling the `src`.
      optimizer: The middleware to use to optimize the circuit. Allowed options are "xls"
            and "yosys". Yosys is the default option and generally produces a smaller circuit.
      num_opt_passes: The number of optimization passes to run on XLS IR (default 1).
            Values <= 0 will skip optimization altogether.
            (Only affects the XLS optimizer.)
      generated_py_file_name: The name of the generated .py file, defaulting to `name`
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
      **kwargs: Keyword arguments to pass through to the py_library target.
    """
    if optimizer not in FHE_OPTIMIZERS:
        fail("Invalid optimizer: ", optimizer)

    valid_lut_sizes = VALID_LUT_SIZES.get(optimizer)
    if not valid_lut_sizes:
        fail("No valid lut sizes configured for ", optimizer, ". Please report a bug.")

    if lut_size not in valid_lut_sizes:
        if len(valid_lut_sizes) == 1:
            lut_size = valid_lut_sizes[0]
        else:
            fail(
                "For optimizer",
                optimizer,
                ", lut_size must be one of",
                valid_lut_sizes,
                "but was",
                lut_size,
            )

    py_file_name = generated_py_file_name or name
    transpiled_xlscc_files = "{}.cc_to_xls_ir".format(name)
    cc_to_xls_ir(
        name = transpiled_xlscc_files,
        library_name = name,
        src = src,
        hdrs = hdrs,
    )

    if optimizer == "xls":
        optimized_intermediate_files = "{}.optimized_bool_ir".format(name)
        xls_ir_to_bool_ir(
            name = optimized_intermediate_files,
            src = ":" + transpiled_xlscc_files,
            num_opt_passes = num_opt_passes,
        )
        py_fhe_bool_ir_library(
            name = name,
            py_file_name = py_file_name,
            src = ":" + optimized_intermediate_files,
            **kwargs
        )
    else:  # optimizer == "yosys"
        verilog = "{}.verilog".format(name)
        xls_ir_to_verilog(
            name = verilog,
            src = ":" + transpiled_xlscc_files,
        )
        netlist = "{}.netlist".format(name)

        cell_library = TFHE_CELLS_LIBERTY
        if lut_size > 0:
            cell_library = LUT_CELLS_LIBERTY

        verilog_to_netlist(
            name = netlist,
            src = ":" + verilog,
            encryption = "jaxite",
            lut_size = lut_size,
            cell_library = cell_library,
        )
        py_fhe_bool_ir_library(
            name = name,
            py_file_name = py_file_name,
            cell_library = cell_library,
            src = ":" + netlist,
            parallelism = parallelism,
            **kwargs
        )
