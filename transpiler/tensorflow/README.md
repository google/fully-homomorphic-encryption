# Experimental: TensorFlow to FHE Inference (TF2FHE) Transpiler

This experimental transpiler enables computing inferences on encrypted data by transpiling TensorFlow Lite models using an [MLIR-based](https://mlir.llvm.org/) compiler, [HEIR](https://github.com/google/heir), and the FHE Transpiler.

## Quick Start

The following dependencies are required.

1. [heir-opt](https://google.github.io/heir/docs/getting_started/).

```bash
git clone git@github.com:google/heir.git && cd heir
bazel build -c opt @heir//tools:all
```

2. [Yosys](https://github.com/YosysHQ/yosys#installation).

3. [FHE Transpiler](https://github.com/google/fully-homomorphic-encryption/tree/main/transpiler).
   The Rust transpiler using the `tfhe-rs` backend has the best performance for these models. It can be built with the following:

```bash
bazel build -c opt //transpiler/rust:transpiler
```

Add these to your PATH to follow the instructions below.

To try out an end to end example, skip to [Hello
World](#hello-world) or [Micro Speech](#micro-speech).

If you would like to transpile an existing TFLite model, you will also need to
install the following tools:

4. [flatbuffer_translate](https://github.com/tensorflow/tensorflow/tree/master/tensorflow/compiler/mlir/lite#the-new-mlir-based-tensorflow-to-tensorflow-lite-converter).

```bash
git clone git@github.com:tensorflow/tensorflow.git && cd tensorflow
bazel build -c opt tensorflow/compiler/mlir/lite:flatbuffer_translate
```

See the instructions [here](https://github.com/tensorflow/tensorflow/tree/master/tensorflow/compiler/mlir#using-local-llvm-repo) to use a local LLVM build,

5. [tf-opt](https://github.com/tensorflow/tensorflow/tree/master/tensorflow/compiler/mlir). For example, use

```bash
bazel build -c opt tensorflow/compiler/mlir:tf-opt
```

## Demos

This section lists the demos included in this repository that are intended to be
simple examples of the TensorFlow Transpiler.

### Hello World

The Hello World example is an example quantized TFLite Model that predicts the
value of a sine wave using three fully connected layers. The first two layers
use the RELU activation function, and the final does not use an activation
function.

This example is small enough to be provided with efficient Bazel target.

To produce the generated files, run the following:

```bash
 # Update with the absolute path to the transpiler repository.
TRANSPILER_ROOT=$(pwd)
export EXAMPLE_DIR=${TRANSPILER_ROOT}/transpiler/tensorflow/examples/hello_world
export LUTMAP_SCRIPT=${TRANSPILER_ROOT}/transpiler/yosys/map_lut_to_lutmux3.v
export LIBERTY_CELLS=${TRANSPILER_ROOT}/transpiler/yosys/lut_cells.liberty

export MODEL_NAME=hello_world
export INPUT_TOSA=${EXAMPLE_DIR}/hello_world.tosa.mlir
export OUTPUT_METADATA=${EXAMPLE_DIR}/hello_world_meta.json
export OUTPUT_VERILOG=${EXAMPLE_DIR}/hello_world.v
export OUTPUT_NETLIST=${EXAMPLE_DIR}/hello_world.netlist.v
export OUTPUT_RUST=${EXAMPLE_DIR}/hello_world_fhe_lib.rs
```

Convert the TOSA MLIR model to Verilog:

```bash
heir-opt --heir-tosa-to-arith ${INPUT_TOSA} | tee >(heir-translate --emit-metadata -o ${OUTPUT_METADATA}) |  heir-translate --emit-verilog -o ${OUTPUT_VERILOG}
```

Run the Yosys optimizer. This first script runs faster (few minutes) but creates
a model with longer inference time.

```bash
YOSYS_SCRIPT="read_verilog ${OUTPUT_VERILOG}; hierarchy -check -top main; techmap; opt; splitnets -ports for_*; abc -lut 3; opt_clean -purge; techmap -map ${LUTMAP_SCRIPT}; opt_clean -purge; flatten; hierarchy -generate lut3 o:Y i:P* i:A i:B i:C; opt_expr; opt; opt_clean -purge; rename -hide */w:*; rename -enumerate */w:*; rename -top ${MODEL_NAME}; clean; write_verilog -noattr ${OUTPUT_NETLIST}"
yosys -p "${YOSYS_SCRIPT}"
```

For the fastest inference but with a longer Yosys processing time (an hour), run
the second script.

```bash
YOSYS_SCRIPT="read_verilog ${OUTPUT_VERILOG}; hierarchy -check -top main; techmap; opt; splitnets -ports for_*; flatten; opt_expr; opt; opt_clean -purge; abc -lut 3; opt_clean -purge; techmap -map ${LUTMAP_SCRIPT}; opt_clean -purge; rename -hide */w:*; rename -enumerate */w:*; rename -top ${MODEL_NAME}; clean; write_verilog -noattr ${OUTPUT_NETLIST}"
yosys -p "${YOSYS_SCRIPT}"
```

Run the tfhe-rs transpiler:

```bash
transpiler --ir_path ${OUTPUT_NETLIST} --liberty_path ${LIBERTY_CELLS} --metadata_path ${OUTPUT_METADATA} --parallelism=0 --rs_out ${OUTPUT_RUST}
```

Then run the testbench with the following:

```bash
bazel run //transpiler/tensorflow/examples/hello_world:hello_world_testbench
```

Note that for models besides Hello World, the resulting netlist and source files
will be very large, and processing and compilation time will take a few hours.

### Micro Speech

The [micro_speech.tosa.mlir](./examples/micro_speech/micro_speech.tosa.mlir) is
a hotword detector that recognizes "yes", "no", "left" and "right". The model
emits scores for each of these labels.

Set the following environment variables to follow the instructions below.

```bash
 # Update with the absolute path to the transpiler repository.
TRANSPILER_ROOT=$(pwd)
export EXAMPLE_DIR=${TRANSPILER_ROOT}/transpiler/tensorflow/examples/micro_speech
export LUTMAP_SCRIPT=${TRANSPILER_ROOT}/transpiler/yosys/map_lut_to_lutmux3.v
export LIBERTY_CELLS=${TRANSPILER_ROOT}/transpiler/yosys/lut_cells.liberty

export MODEL_NAME=micro_speech
export OUTPUT_DIR=$(mktemp -d -t ${MODEL_NAME}_XXX)
cp -r ${EXAMPLE_DIR}/* ${OUTPUT_DIR}
export INPUT_TOSA=${OUTPUT_DIR}/micro_speech.tosa.mlir
export OUTPUT_METADATA=${OUTPUT_DIR}/micro_speech_meta.json
export OUTPUT_VERILOG=${OUTPUT_DIR}/micro_speech.v
export OUTPUT_NETLIST=${OUTPUT_DIR}/micro_speech.netlist.v
export OUTPUT_RUST=${OUTPUT_DIR}/src/micro_speech/micro_speech_fhe_lib.rs
```

1. Convert the model to Verilog

Use `heir-opt` and `heir-translate` to convert the model to Verilog and emit
metadata for the transpiler.

```bash
heir-opt --heir-tosa-to-arith ${INPUT_TOSA} | tee >(heir-translate --emit-metadata -o ${OUTPUT_METADATA}) |  heir-translate --emit-verilog -o ${OUTPUT_VERILOG}
```

2. Yosys Optimization

Note: This step will take about 30 min to complete. This is a known issue.

```bash
YOSYS_SCRIPT="read_verilog ${OUTPUT_VERILOG}; hierarchy -check -top main; techmap; opt; splitnets -ports for_*; abc -lut 3 -fast; opt_clean -purge; techmap -map ${LUTMAP_SCRIPT}; opt_clean -purge; flatten; hierarchy -generate lut3 o:Y i:P* i:A i:B i:C; opt_expr; opt; opt_clean -purge; rename -hide */w:*; rename -enumerate */w:*; rename -top ${MODEL_NAME}; clean; write_verilog -noattr ${OUTPUT_NETLIST}"
yosys -p "${YOSYS_SCRIPT}"
```

3. Rust Transpiler

```bash
transpiler --ir_path ${OUTPUT_NETLIST} --liberty_path ${LIBERTY_CELLS} --metadata_path ${OUTPUT_METADATA} --parallelism=0 --rs_out ${OUTPUT_RUST}
```

4. Run the testbench

Note: The Rust compilation time will take a few hours. This is a known issue.

```bash
cd ${OUTPUT_DIR}
cargo run --release
```

## Limitations

This sections describes the known limitations of the TensorFlow Transpiler.

* FHE programming requires fixed-length tensors. One workaround is to use
  tensors with the largest number of values needed (pad the tensor).
* Floating-point data types are not supported. Models must be quantized (see
  [Post-training
  quantization](https://www.tensorflow.org/lite/performance/post_training_quantization)
  and [Quantization Aware
  Training](https://www.tensorflow.org/model_optimization/guide/quantization/training)).

The following limitations are due to our existing tools and dependencies:

* The model must be able to be lowered to TOSA MLIR. Operations such as
  `tfl.custom` are not supported.
* Named arguments are not supported due to our intermediate representation with
  MLIR and the Verilog translation.
* TFLite can only quantize to
  [8-bits](https://www.tensorflow.org/lite/performance/quantization_spec), but
  it may be possible to use [Brevitas](https://github.com/Xilinx/brevitas) and
  lower to the TOSA MLIR dialect using
  [Torch-MLIR](https://github.com/llvm/torch-mlir).


### Transpiling your TensorFlow Lite model

To run the TensorFlow Transpiler on your own TensorFlow Lite model, use the
following steps. Note that the model must not have variable-length tensors and
must be quantized. Create a directory for the output files and set the following
environment variables:

```bash
export INPUT_TFLITE=model.tflite
export MODEL_NAME=${INPUT_TFLITE%.tflite}
export OUTPUT_DIR=$(mktemp -d -t ${MODEL_NAME}_XXX)
export OUTPUT_TOSA=${OUTPUT_DIR}/${MODEL_NAME}.tosa.mlir
export OUTPUT_METADATA=${OUTPUT_DIR}/${MODEL_NAME}_meta.json
export OUTPUT_VERILOG=${OUTPUT_DIR}/${MODEL_NAME}.verilog
export OUTPUT_NETLIST=${OUTPUT_DIR}/${MODEL_NAME}.netlist.v

mkdir -p ${OUTPUT_DIR}/src/${MODEL_NAME}
export OUTPUT_RUST=${OUTPUT_DIR}/src/${MODEL_NAME}/${MODEL_NAME}_fhe_lib.rs
export TESTBENCH_FILE=${OUTPUT_DIR}/src/main.rs
```

1. Convert the TensorFlow Lite model to TOSA MLIR.

```bash
flatbuffer_translate --tflite-flatbuffer-to-mlir ${INPUT_TFLITE} | tf-opt --tf-tfl-to-tosa-pipeline --tosa-strip-quant-types -o ${OUTPUT_TOSA}
```

2. Run steps 1-3 of the [Micro Speech](#micro-speech) example above.

4. Setup a Cargo project. Add the following `Cargo.toml` in `${OUTPUT_DIR}` with
   the string substitutions:

```toml
[package]
name = "${MODEL_NAME}"
version = "0.1.0"
edition = "2021"

[dependencies]
rayon = "1.6.1"
tfhe = { version = "0.2.4", features = ["boolean", "shortint", "x86_64-unix"] }
```

5. Create a testbench file `${TESTBENCH_FILE}` that invokes the transpiled model
   in the `${OUTPUT_RUST}` file. You can check the function signature from the
   file using

```bash
grep "pub fn ${MODEL_NAME}" ${OUTPUT_RUST}
```

Run the testbench with:

```bash
cd ${OUTPUT_DIR}
cargo run --release
```

## Future Work

In the near future, we intend to add additional optimizations that will reduce
the size of the generated netlist and source and speed up execution time by
pipelining loops. This will also improve compilation time. Currently, the Rust
transpiler using LUTs yields the best performance.

We also plan to integrate with other MLIR-based FHE efforts, including the
[CONCRETE compiler](https://github.com/zama-ai/concrete).


