# Hotword Keyword Spotting CKKS FHE Demo

This directory contains demonstrations for training and evaluating a temporal
convolutional neural network (TC-ResNet) for keyword spotting / wake-word
detection on the Google Speech Commands dataset using Fully Homomorphic
Encryption (FHE) with the CKKS scheme, compiled via HEIR.

This demo implements TC-ResNet, a tiny yet accurate model designed for on-device
keyword spotting. The model is trained on the Google Speech Commands dataset,
which consists of 1-second audio clips of spoken words. The goal is to classify
the input audio into one of 12 classes: 10 core keywords ("yes", "no", "up",
"down", "left", "right", "on", "off", "stop", "go"), "silence", and "unknown".

For more details on the model, see the original paper:
S Choi et. al. (2019). [Temporal convolution for real-time keyword spotting on mobile devices](https://arxiv.org/abs/1904.03814)

This demo includes evaluations using Lattigo (Go) backend.

**Warning:** The demos in this directory require a lot of RAM! If your machine
doesn't have at least 96 GiB of RAM, you can run them by configuring swap space,
but the result will be significantly slower.

## Directory Structure

```
.
├── BUILD
├── README.md
├── cleartext/                  # Unencrypted Python baseline
├── data/                       # Pre-trained models, test samples, and MLIR models
├── lattigo/                    # Go FHE evaluation targets
├── torch/                      # PyTorch model and export scripts
├── train/                      # PyTorch training script
└── utils/                      # Data preparation and encoding utility scripts
```

## Dataset Preparation

The training script requires the Google Speech Commands dataset (v0.02 is
recommended). Download and extract it to a directory:

```bash
# Create a directory for the dataset
mkdir speech_commands

# Download the dataset
wget http://download.tensorflow.org/data/speech_commands_v0.02.tar.gz

# Extract the dataset
tar -xf speech_commands_v0.02.tar.gz -C speech_commands
```

A small subset of test data is already included in `data/test_data.npz` and
`data/test_data-small.npz` for evaluation.

## Training

To train the TC-ResNet8 model on CPU:

```bash
bazel run //demos/hotword/train:train_tc_resnet -- \
    --data_dir=/path/to/speech_commands \
    --out_model=$PWD/demos/hotword/data/tc_resnet8.pth \
    --out_test_data=$PWD/demos/hotword/data/test_data.npz
```

## Cleartext Evaluation

To run unencrypted baseline inference in Python:

*   **Single Sample Evaluation:**

    ```bash
    bazel run //demos/hotword/cleartext:evaluate_cleartext -- --sample_idx=0
    ```

*   **Batched Suite Evaluation:**

    ```bash
    bazel run //demos/hotword/cleartext:evaluate_cleartext_suite
    ```

## FHE Evaluation

We support FHE evaluation using Lattigo (Go).

> [!IMPORTANT]
> FHE evaluations are computationally expensive. It is highly recommended to run them with optimized compilation mode (`-c opt`) to ensure reasonable execution times.

### Lattigo (Go)

*   **Single Sample Evaluation:**

    ```bash
    bazel run -c opt //demos/hotword/lattigo:evaluate_fhe -- --sample_idx=0
    ```

*   **Batched Suite Evaluation:**

    ```bash
    bazel run -c opt //demos/hotword/lattigo:evaluate_fhe_suite
    ```

*   **Timing Evaluation:**

    ```bash
    bazel run -c opt //demos/hotword/lattigo:evaluate_fhe_timing -- --sample_idx=0
    ```
