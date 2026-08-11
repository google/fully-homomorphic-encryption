# Criteo HELRM CKKS FHE Demo

This directory contains demonstrations for evaluating a Homomorphic Encryption
Logistic Regression Model (HELRM) on the Criteo dataset using Fully Homomorphic
Encryption (FHE) with the CKKS scheme, compiled via HEIR.

This demo includes evaluations using Lattigo (Go) backend.

**Warning:** The demos in this directory require a lot of RAM! If your machine
doesn't have at least 96 GiB of RAM, you can run them by configuring swap space,
but the result will be significantly slower.

## Directory Structure

```
.
├── README.md
├── cleartext/                  # Unencrypted Python baseline
├── data/                       # Pre-trained models, test samples, and MLIR models
├── lattigo/                    # Go FHE evaluation targets
├── torch/                      # PyTorch model and export scripts
└── utils/                      # Data preparation and encoding utility scripts
```

## Dataset

The demo can be run with a sample dataset. A pre-generated sample is included in
`data/sample.pt`. Note we do not provide a training script for this model
because the dataset is too large for a demo repository like this one (1 TiB).

### Downloading and Preparing the Full Dataset

To run with real data from the Criteo 1TB ad-click dataset:

1.  **Download a single part of `day=2015-02-15` (approx 97 MB):**

    ```bash
    curl -L -o demos/criteo/part-00015.parquet \
      "https://huggingface.co/datasets/criteo/CriteoClickLogs/resolve/main/data/day=2015-02-15/part-00015-99c339d5-fbac-4110-9dcf-75453a61a5c1.c000.snappy.parquet"
    ```

2.  **Convert the Parquet file to raw TSV format:**

    ```bash
    bazel run //demos/criteo/utils:convert_criteo_parquet -- \
      --input=$(pwd)/demos/criteo/part-00015.parquet \
      --output=$(pwd)/demos/criteo/part-00015.txt
    ```

3.  **Extract a sample for testing:**

    ```bash
    bazel run //demos/criteo/utils:sample_data -- \
      --raw_data=$(pwd)/demos/criteo/part-00015.txt \
      --output=$(pwd)/demos/criteo/data/sample.pt \
      --num_samples=20
    ```

    This will preprocess the data and save it to `demos/criteo/data/sample.pt`,
    overwriting the default sample.

## Cleartext Evaluation

To run unencrypted baseline inference in Python:

```bash
bazel run //demos/criteo/cleartext:evaluate_cleartext
```

This script runs inference on synthetic inputs matching the FHE evaluation.

## FHE Evaluation

We support FHE evaluation using Lattigo (Go).

> [!IMPORTANT]
> FHE evaluations are computationally expensive. It is highly recommended to run them with optimized compilation mode (`-c opt`) to ensure reasonable execution times.

### Lattigo (Go)

*   **Evaluation:**

    ```bash
    bazel run -c opt //demos/criteo/lattigo:evaluate_fhe
    ```

    This binary runs FHE inference using synthetic inputs.

## Running Tests

To run the PyTorch inference test which validates the model accuracy on the sample data:

```bash
bazel test //demos/criteo/torch:criteo_inference_test
```
