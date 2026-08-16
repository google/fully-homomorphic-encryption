# Credit Card Fraud Detection CKKS FHE Demo

This directory contains code for evaluating a pre-trained PyTorch MLP neural
network model used for detecting credit card fraud against equivalent Fully
Homomorphic Encryption (FHE) variants.

HEIR was used to compile the Torch MLP model (Linear -> Sigmoid -> Linear ->
Sigmoid -> Linear) with the CKKS scheme.

This demo includes evaluations using both Lattigo (Go) and OpenFHE (C++/Python)
backends. It also includes "timing" and "debug" variants to measure performance
and verify intermediate values against a cleartext reference.

## Directory Structure

```
.
├── BUILD
├── README.md
├── cleartext/                  # Unencrypted Python baseline
├── data/                       # Pre-trained models, test samples, and MLIR models
├── debug/                      # Debug reference generation and MLIR models
├── lattigo/                    # Go FHE evaluation targets
├── openfhe/                    # OpenFHE FHE evaluation targets
├── torch/                      # PyTorch model and export scripts
├── train/                      # PyTorch training script
└── utils/                      # Data preparation and encoding utility scripts
```

## Dataset

The demo uses a credit card fraud detection dataset. The pre-trained model and
test samples are included in the `data/` directory:

*   `data/mlp_fraud_model_sigmoid.pt`: Pre-trained PyTorch model.
*   `data/test_rows.csv`: A sample of 20 test rows (10 fraud, 10 non-fraud) extracted
    from the dataset.
*   `data/feature_cols.pkl`: Pickle file containing the list of 82 feature columns
    used by the model.

### Training Data

To retrain the model or extract new test rows, you need to download and
preprocess the raw dataset:

1.  **Download the raw dataset:** Download the Credit Card Fraud Detection
    dataset from Kaggle:
    https://www.kaggle.com/datasets/kartik2112/fraud-detection This will give
    you `fraudTrain.csv` and `fraudTest.csv`.

2.  **Prep the data:** Run the prep script to merge the CSVs and extract
    temporal and age features:

    ```bash
    bazel run //demos/cc_fraud/utils:prep_data -- \
      --train_csv /absolute/path/to/fraudTrain.csv \
      --test_csv /absolute/path/to/fraudTest.csv \
      --output /absolute/path/to/sparkov_fraud_prepped.parquet
    ```

    Or run the binary directly from the workspace root:

    ```bash
    ./bazel-bin/demos/cc_fraud/utils/prep_data \
      --train_csv /absolute/path/to/fraudTrain.csv \
      --test_csv /absolute/path/to/fraudTest.csv \
      --output demos/cc_fraud/data/sparkov_fraud_prepped.parquet
    ```

3.  **Encode the data:** Run the encoding script to apply ordinal encoding,
    one-hot encoding, and scaling:

    ```bash
    bazel run //demos/cc_fraud/utils:encode_data -- \
      --input /absolute/path/to/sparkov_fraud_prepped.parquet \
      --output /absolute/path/to/sparkov_fraud_encoded.parquet \
      --mappings /absolute/path/to/demos/cc_fraud/data/encoder_mappings.json \
      --scaler /absolute/path/to/demos/cc_fraud/data/scaler.pkl \
      --feature_cols /absolute/path/to/demos/cc_fraud/data/feature_cols.pkl
    ```

    Or run the binary directly from the workspace root with default paths:

    ```bash
    ./bazel-bin/demos/cc_fraud/utils/encode_data
    ```

    This will generate `sparkov_fraud_encoded.parquet`, which is used by the
    training and inference scripts.

4.  **Extract test rows (optional):** To create a small CSV sample for testing:

    ```bash
    bazel run //demos/cc_fraud/utils:extract_test_rows -- \
      --input /absolute/path/to/sparkov_fraud_encoded.parquet \
      --output /absolute/path/to/test_rows.csv \
      --feature_cols /absolute/path/to/demos/cc_fraud/data/feature_cols.pkl \
      --num_fraud 10 \
      --num_non_fraud 10
    ```

    Or run the binary directly from the workspace root:

    ```bash
    ./bazel-bin/demos/cc_fraud/utils/extract_test_rows
    ```

## Cleartext Evaluation

The cleartext directory provides two binaries for unencrypted baseline evaluation:

### Single Sample Evaluation

To evaluate a single sample from the test dataset:

```bash
bazel run //demos/cc_fraud/cleartext:evaluate_cleartext -- \
  --sample_idx 0 \
  --model_path /absolute/path/to/demos/cc_fraud/data/mlp_fraud_model_sigmoid.pt \
  --feature_cols_path /absolute/path/to/demos/cc_fraud/data/feature_cols.pkl \
  --data_path /absolute/path/to/demos/cc_fraud/data/test_rows.csv
```

Or run the binary directly from the workspace root with default paths:

```bash
./bazel-bin/demos/cc_fraud/cleartext/evaluate_cleartext --sample_idx 0
```

This evaluates a single sample and prints the prediction, true label, fraud
probability, and latency.

### Activation Range Estimation (Calibration)

To estimate activation ranges for FHE parameter selection:

```bash
bazel run //demos/cc_fraud/cleartext:estimate_ranges -- \
  --model_path /absolute/path/to/demos/cc_fraud/data/mlp_fraud_model_sigmoid.pt \
  --feature_cols_path /absolute/path/to/demos/cc_fraud/data/feature_cols.pkl \
  --data_path /absolute/path/to/demos/cc_fraud/data/test_rows.csv
```

Or run the binary directly from the workspace root with default paths:

```bash
./bazel-bin/demos/cc_fraud/cleartext/estimate_ranges
```

This script loads the pre-trained model, runs inference on the test set, and
prints validation metrics and activation ranges for each layer. The activation
ranges help determine appropriate scaling parameters for FHE encryption.

## FHE Evaluation

We support FHE evaluation using Lattigo (Go) and OpenFHE (C++/Python via
pybind).

> [!IMPORTANT] FHE evaluations are computationally expensive. It is highly
> recommended to run them with optimized compilation mode (`-c opt`) to ensure
> reasonable execution times.

For both backends, there are three variants:

1.  **Standard**: Standard FHE evaluation.
2.  **Timing**: Measures and prints the execution time of each FHE operator.
3.  **Debug**: Decrypts intermediate values after key operations and compares
    them against a cleartext reference to check precision.

### Lattigo (Go)

*   **Single Sample Evaluation (Standard):**

    ```bash
    bazel run -c opt //demos/cc_fraud/lattigo:evaluate_fhe -- --row_idx=0
    ```

*   **Batched Suite Evaluation (Standard):**

    ```bash
    bazel run -c opt //demos/cc_fraud/lattigo:evaluate_fhe_suite
    ```

*   **Timing Evaluation:**

    ```bash
    bazel run -c opt //demos/cc_fraud/lattigo:evaluate_fhe_timing -- --row_idx=0
    ```

*   **Debug Evaluation:**

    ```bash
    # Set the environment variable to specify which row to debug (default is 0)
    HEIR_DEBUG_ROW_IDX=0 bazel run -c opt //demos/cc_fraud/lattigo:evaluate_fhe_debug
    ```

### OpenFHE (C++/Python)

*   **Single Sample Evaluation (Standard):**

    ```bash
    bazel run -c opt //demos/cc_fraud/openfhe:evaluate_fhe -- --row_idx=0
    ```

*   **Batched Suite Evaluation (Standard):**

    ```bash
    bazel run -c opt //demos/cc_fraud/openfhe:evaluate_fhe_suite
    ```

*   **Timing Evaluation:**

    ```bash
    bazel run -c opt //demos/cc_fraud/openfhe:evaluate_fhe_timing -- --row_idx=0
    ```

*   **Debug Evaluation:**

    ```bash
    bazel run -c opt //demos/cc_fraud/openfhe:evaluate_fhe_debug -- --row_idx=0
    ```

    *Note: The debug helper for OpenFHE also reads `HEIR_DEBUG_ROW_IDX` env var,
    which is automatically set by the python wrapper based on the `--row_idx`
    flag.*

## Developer Tools

If you modify the model or test data, you may need to regenerate the debug
reference data:

1.  **Generate JSON reference data:**

    ```bash
    bazel run //demos/cc_fraud/debug:generate_debug_reference
    ```

    This generates `debug/debug_reference.json` (used by Lattigo debug).

2.  **Generate C++ header reference data:**

    ```bash
    bazel run //demos/cc_fraud/debug:generate_debug_reference_hdr
    ```

    This converts `debug_reference.json` to `openfhe/debug_reference.h` (used by
    OpenFHE debug).
