# HEIR demos

This directory contains a set of demos using HEIR to compile pre-trained
PyTorch models to FHE. All of the models are pre-compiled with scripts
providing command to quickly run and assess model performance.

## MNIST

The classic [MNIST](https://en.wikipedia.org/wiki/MNIST_database) digit
identification problem.

- 3-layer MLP neural network
- CKKS FHE scheme
- Example run: `bazel run //demos/mnist/lattigo:evaluate_single`
- [More examples](https://github.com/google/fully-homomorphic-encryption/blob/main/demos/mnist/README.md)

## Credit Card Fraud

A MLP model (Linear -> Sigmoid -> Linear ->
Sigmoid -> Linear) for detecting credit card fraud using
[Kaggle fraud-detection data](https://www.kaggle.com/datasets/kartik2112/).

- CKKS FHE scheme
- Example run: `bazel run -c opt //demos/cc_fraud/lattigo:evaluate_fhe -- --row_idx=0`
- [More Examples](https://github.com/google/fully-homomorphic-encryption/blob/main/demos/cc_fraud/README.md)

## Network Anomaly Detection

A Network Anomaly model is an ensemble anomaly detector composed of an
ensemble layer of multiple parallel autoencoders and a final output
anomaly detector layer.

- CKKS FHE scheme
- Example run: `bazel run -c opt //demos/network_anomaly/lattigo:evaluate_lattigo -- --sample_idx 0`
- [More Examples](https://github.com/google/fully-homomorphic-encryption/blob/main/demos/network_anomaly/README.md)

## Hotword Keyword Spotting

A temporal convolutional neural network (TC-ResNet8) for keyword
spotting / wake-word detection.

- TC-ResNet8 model trained on Speech Commands dataset.
- CKKS FHE scheme.
- Example run: `bazel run //demos/hotword/lattigo:evaluate_single`
- [More examples](https://github.com/google/fully-homomorphic-encryption/blob/main/demos/hotword/README.md)

## Private Content Recoomendation

 Deep Learning Recommendation Model, Homomorphic Encryption Logistic Regression Model (HELRM),that unlocks serving private content recommendations.

- DLRM (Deep Learning Recommendation Model) with a layout consisting of
a bottom branch for dense features, an embedding branch for sparse features,
an interaction step (addition), and a top MLP to produce the final prediction.
- CKKS FHE Scheme
- Example run: `bazel run -c opt //demos/criteo/lattigo:evaluate_fhe`
- [More Examples](https://github.com/google/fully-homomorphic-encryption/blob/main/demos/criteo/README.md)

# Exporting torch to MLIR

The process of exporting a PyTorch model to work with HEIR is not yet automated.
The process involves:

1. Selecting a torch model specification and pre-trained model file.
2. Using [`torch-mlir`](https://github.com/llvm/torch-mlir) to export the torch
   model and frozen weights to an MLIR file (cf.
   `common/python/export_mlir_utils.py`).
3. Writing a script that outputs range estimates for the inputs to all
   activation functions (e.g., ReLU or sigmoid). Cf.
   `demos/hotword/cleartext/calibrate.py` for an example.
4. Annotating the MLIR ops (`linalg.generic`) that compute the activations
   with range bound and a choice of polynomial degree (which controls the
   accuracy of the approxmiation and allows one to trade off performance for
   accuracy. Cf. `demos/cc_fraud/data/model_annotated.mlir` for an example. The
   annotation has syntax like:

```mlir
    %5 = linalg.generic {
        degree = 7 : i32,
        domain_lower = -10.000000e+00 : f64,
        domain_upper = 10.000000e+00 : f64,
    ...} ins(%4 : tensor<1x128xf32>) outs(%1 : tensor<1x128xf32>) {
    ... <activation ops> ...
    } -> tensor<1x128xf32>
```

After you have an exported MLIR file with annotations, it can then be given
as the `mlir_src` argument to a `rules_heir` macro like `heir_lattigo_lib`.
