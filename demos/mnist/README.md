# MNIST CKKS FHE Demo

This directory contains demonstrations for evaluating a pre-trained Py Torch
3-layer MLP neural network model on MNIST images using Fully Homomorphic
Encryption (FHE) with the CKKS scheme, compiled via HEIR.

```
.
└── mnist
    ├── README.md
    ├── cleartext
    ├── data
    ├── lattigo
    ├── openfhe
    ├── torch
    └── utils
```

## Cleartext Evaluation

To run unencrypted baseline inference in Python:

*   **Single Sample Evaluation:**

    ```bash
    bazel run //demos/mnist/cleartext:evaluate_cleartext -- --sample_idx=0
    ```

*   **Batched Suite Evaluation:**

    ```bash
    bazel run //demos/mnist/cleartext:evaluate_cleartext_suite
    ```

## Lattigo (Go) FHE Evaluation

To run FHE evaluation using the Lattigo Go backend:

*   **Single Sample Evaluation:**

    ```bash
    bazel run //demos/mnist/lattigo:evaluate_fhe -- --sample_idx=0
    ```

*   **Batched Suite Evaluation:**

    ```bash
    bazel run //demos/mnist/lattigo:evaluate_fhe_suite
    ```

## OpenFHE (C++/Python) FHE Evaluation

To run FHE evaluation using the OpenFHE C++ backend via Python bindings:

*   **Single Sample Evaluation:**

    ```bash
    bazel run //demos/mnist/openfhe:evaluate_fhe -- --sample_idx=0
    ```

*   **Batched Suite Evaluation:**

    ```bash
    bazel run //demos/mnist/openfhe:evaluate_fhe_suite
    ```
