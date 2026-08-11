# Network Anomaly Detection FHE Demo (KitNET)

This directory contains a PyTorch and Lattigo (Go) implementation of the
**KitNET** (Kitsune) ensemble anomaly detector, optimized for Fully Homomorphic
Encryption (FHE) with Google's **HEIR** compiler.

This demo is based on Niobium's
[fhe-NetworkMonitor](https://github.com/NiobiumInc/fhe-NetworkMonitor) and the
original Kitsune anomaly detector by
[Mirsky et al.](https://github.com/ymirsky/Kitsune-py/tree/master/KitNET).

---

## 1. Model Architectures Overview

We support two distinct KitNET model configurations:

| Architecture Profile | Features | Ensemble Layer | Output Layer | Dataset File | Weights Checkpoint |
| :--- | :---: | :--- | :--- | :--- | :--- |
| **5-Feature KitNET** *(Mini Baseline)* | 5 | 2 sub-AEs (2&rarr;1&rarr;2, 3&rarr;2&rarr;3) | 5&rarr;3&rarr;5 AutoEncoder | `Mirai_first_batch_32K.bin` *(40 B/pkt)* | `torch_kitnet_model.pt` |
| **50-Feature KitNET** *(Full Unified)* | 50 | 5 sub-AEs unified block-diagonal (50&rarr;40&rarr;50) | 50&rarr;5&rarr;50 AutoEncoder | `Mirai_full_50_features_32K.bin` *(400 B/pkt)* | `torch_50_kitnet_model.pt` |

- **5-Feature Model**: Hierarchical ensemble baseline for rapid verification and latency profiling.
- **50-Feature Model**: Uses a **Unified Block-Diagonal Linear Transformation** for the ensemble layer to eliminate `tensor.concat` overhead in FHE compilation.

---

## 2. Directory Structure

```
network_anomaly/
├── BUILD                           # Package definition
├── README.md                       # Documentation & run instructions
├── data/                           # Datasets, labels, checkpoints & MLIR
│   ├── BUILD
│   ├── Mirai_first_batch_32K.bin   # 5-feature baseline packet dataset
│   ├── Mirai_full_50_features_32K.bin # 50-feature full packet dataset
│   ├── Mirai_labels.csv            # Ground-truth binary labels (764K rows)
│   ├── torch_kitnet_model.pt       # Trained 5-feature PyTorch weights
│   ├── torch_50_kitnet_model.pt    # Trained 50-feature PyTorch weights
│   ├── torch_kitnet_model_annotated.mlir # Annotated 5-feature MLIR
│   └── torch_50_kitnet_model_annotated.mlir # Annotated 50-feature MLIR
├── torch/                          # PyTorch models & export tooling
│   ├── BUILD
│   ├── pytorch_kitnet.py           # 5-feature hierarchical KitNET model
│   ├── pytorch_50_kitnet.py        # 50-feature Unified Block-Diagonal model
│   └── export_mlir.py              # Exporter using demos/common helpers
├── train/                          # PyTorch training binaries
│   ├── BUILD
│   ├── train_pytorch_kitnet.py     # 5-feature Adam trainer
│   └── train_50_pytorch_kitnet.py  # 50-feature Adam trainer (block-masked)
├── cleartext/                      # Cleartext evaluation & confusion matrix
│   ├── BUILD
│   ├── evaluate_single.py          # 5-feature single sample runner
│   ├── evaluate_suite.py           # 5-feature multi-sample evaluation
│   ├── evaluate_50_single.py       # 50-feature single sample runner
│   └── evaluate_50_suite.py        # 50-feature multi-sample evaluation
└── lattigo/                        # FHE evaluation via HEIR-generated Lattigo
    ├── BUILD                       # Uses heir_lattigo_lib rule
    ├── evaluate_fhe.go             # Single packet sample FHE evaluation
    ├── evaluate_fhe_suite.go       # Multi-sample FHE evaluation
    ├── evaluate_fhe_timing.go      # Breakdown phase latency benchmarking
    ├── timing_helper.go            # Wrapper over demos/common/lattigo/debug
    └── utils.go                    # Data & label loaders using pathutils
```

---

## 3. How to Build and Run

### 3.1 Build All Targets
```bash
SKYBUILD=1 bazel build //demos/network_anomaly/...
```

### 3.2 Cleartext Model Evaluation
Run 5-feature cleartext evaluation across the dataset:
```bash
bazel run //demos/network_anomaly/cleartext:evaluate_suite -- --num_samples 1000
```

Run 50-feature cleartext evaluation:
```bash
bazel run //demos/network_anomaly/cleartext:evaluate_50_suite -- --num_samples 1000
```

### 3.3 Lattigo FHE Homomorphic Inference
Evaluate a single packet sample under FHE encryption:
```bash
bazel run //demos/network_anomaly/lattigo:evaluate_fhe -- --sample_idx 0
```

Evaluate a multi-sample batch under FHE and compute confusion matrix:
```bash
bazel run //demos/network_anomaly/lattigo:evaluate_fhe_suite -- --num_samples 10
```

Benchmark FHE timing across encryption, evaluation, and decryption phases:
```bash
bazel run //demos/network_anomaly/lattigo:evaluate_fhe_timing -- --runs 3
```

### 3.4 Model Training & MLIR Export
Train a new 5-feature model checkpoint:
```bash
bazel run //demos/network_anomaly/train:train_pytorch_kitnet
```

Train a new 50-feature model checkpoint:
```bash
bazel run //demos/network_anomaly/train:train_50_pytorch_kitnet
```

Export PyTorch models to annotated MLIR:
```bash
bazel run //demos/network_anomaly/torch:export_mlir -- --model_type 50 --output model_50_annotated.mlir
```

---

## 4. How to Interpret and Understand Results

### 4.1 Reconstruction Error & Anomaly Scores
- **Sum of Squared Errors (SSE)**: Total squared error between input features $x$ and reconstructed output $\hat{x}$:
  $$\text{SSE} = \sum_{i=1}^{D} (x_i - \hat{x}_i)^2$$
- **Mean Squared Error (MSE)**: Dimension-normalized anomaly score:
  $$\text{MSE} = \frac{\text{SSE}}{D} \quad (D = 5 \text{ or } 50)$$

### 4.2 Classification Decision Logic

- **$\text{MSE} \ge \text{Threshold}$** &rarr; Flagged as **`ANOMALY`** (attack/intrusion traffic, e.g. Mirai botnet attack).
- **$\text{MSE} < \text{Threshold}$** &rarr; Flagged as **`BENIGN`** (normal, clean traffic).

*Recommended thresholds:* `0.005` for 5-feature baseline, `0.0001` - `0.005` for 50-feature model.

### 4.3 Confusion Matrix & Performance Metrics

- **True Positives (TP)**: Attack packets correctly flagged as ANOMALY.
- **True Negatives (TN)**: Benign packets correctly classified as BENIGN.
- **False Positives (FP)**: False alarms (benign packets flagged as ANOMALY).
- **False Negatives (FN)**: Missed intrusions (attack packets classified as BENIGN).
- **Accuracy**: $\frac{\text{TP} + \text{TN}}{\text{Total}} \times 100\%$
- **Specificity**: $\frac{\text{TN}}{\text{TN} + \text{FP}} \times 100\%$ (ability to avoid false alarms).
- **False Positive Rate (FPR)**: $\frac{\text{FP}}{\text{TN} + \text{FP}} \times 100\%$
