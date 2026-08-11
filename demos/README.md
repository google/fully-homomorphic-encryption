# HEIR demos

This directory contains a set of demos using HEIR to compile pre-trained PyTorch
models to FHE.

## MNIST

The classic [MNIST](https://en.wikipedia.org/wiki/MNIST_database) digit
identification problem.

- 3-layer MLP neural network
- CKKS FHE scheme
- Example run: `bazel run //demos/mnist/lattigo:evaluate_single`
- [More examples](https://github.com/google/fully-homomorphic-encryption/blob/main/demos/mnist/README.md)

## Hotword Keyword Spotting

A temporal convolutional neural network (TC-ResNet8) for keyword spotting / wake-word detection.

- TC-ResNet8 model trained on Speech Commands dataset.
- CKKS FHE scheme.
- Example run: `bazel run //demos/hotword/lattigo:evaluate_single`
- [More examples](https://github.com/google/fully-homomorphic-encryption/blob/main/demos/hotword/README.md)
