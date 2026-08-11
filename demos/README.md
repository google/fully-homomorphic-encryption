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
