"""Batched cleartext evaluation and validation suite runner for PyTorch KitNET."""

import argparse
import csv
import os
import sys
import time
from typing import List, Optional

import numpy as np
import torch

from demos.common.python import path_utils
from demos.network_anomaly.torch import pytorch_kitnet

PyTorchKitNET = pytorch_kitnet.PyTorchKitNET
resolve_path = path_utils.resolve_path


def load_dataset_suite(
    data_file: str, num_samples: int, num_features: int
) -> torch.Tensor:
  """Loads packet feature vectors from binary dataset."""
  resolved_data_path = resolve_path(data_file)
  if not os.path.exists(resolved_data_path):
    raise FileNotFoundError(f"Dataset file not found at {data_file}")

  with open(resolved_data_path, "rb") as f:
    raw_bytes = f.read(num_samples * num_features * 8)
    num_read = len(raw_bytes) // (num_features * 8)
    data_np = (
        np.frombuffer(raw_bytes, dtype=np.float64)
        .reshape((num_read, num_features))
        .copy()
    )

  return torch.from_numpy(data_np).float()


def load_labels(labels_file: str, max_samples: int) -> Optional[np.ndarray]:
  """Loads ground truth labels (0=benign, 1=anomaly) from CSV file."""
  resolved_labels_path = resolve_path(labels_file)
  if not os.path.exists(resolved_labels_path):
    print(
        f"Notice: Labels file not found at {labels_file}, skipping label"
        " validation."
    )
    return None

  labels: List[int] = []
  try:
    with open(resolved_labels_path, "r", encoding="utf-8") as f:
      reader = csv.reader(f)
      _ = next(reader, None)
      for row in reader:
        if len(row) >= 2:
          try:
            labels.append(int(float(row[1])))
          except ValueError:
            continue
        elif len(row) == 1:
          try:
            labels.append(int(float(row[0])))
          except ValueError:
            continue
        if len(labels) >= max_samples:
          break
  except Exception as e:  # pylint: disable=broad-except
    print(f"Warning: Failed to parse labels file: {e}")
    return None

  return np.array(labels, dtype=np.int32)


def evaluate_suite(
    model_path: str = "demos/network_anomaly/data/torch_kitnet_model.pt",
    data_file: str = "demos/network_anomaly/data/Mirai_first_batch_32K.bin",
    labels_file: Optional[
        str
    ] = "demos/network_anomaly/data/Mirai_labels.csv",
    num_samples: int = 32768,
    num_features: int = 5,
    batch_size: int = 64,
    threshold: Optional[float] = None,
) -> None:
  """Evaluates PyTorch KitNET model over dataset suite and validates confusion matrix."""
  resolved_model_path = resolve_path(model_path)
  resolved_data_path = resolve_path(data_file)

  print("=" * 80)
  print("  PyTorch KitNET Cleartext Suite Evaluation & Ground Truth Validation")
  print("=" * 80)
  print(f"Model Checkpoint:  {resolved_model_path}")
  print(f"Packet Dataset:    {resolved_data_path}")
  if labels_file:
    print(f"Labels File:       {resolve_path(labels_file)}")
  print(f"Requested Samples: {num_samples}")
  print(f"Features / Sample: {num_features}")
  print(f"Batch Size:        {batch_size}\n")

  # 1. Load Model
  if resolved_model_path.endswith(".bin"):
    model = PyTorchKitNET.load_from_binary_model(resolved_model_path)
  else:
    model = PyTorchKitNET.load_weights(resolved_model_path)
  model.eval()

  # 2. Load Dataset
  t0 = time.perf_counter()
  data_tensor = load_dataset_suite(data_file, num_samples, num_features)
  actual_samples = data_tensor.shape[0]
  print(
      f"[1/3] Loaded {actual_samples} packet vectors in"
      f" {time.perf_counter() - t0:.4f} s"
  )

  if actual_samples == 0:
    print("Error: No packet samples loaded.")
    return

  # 3. Load Labels
  labels: Optional[np.ndarray] = None
  if labels_file:
    labels = load_labels(labels_file, actual_samples)
    if labels is not None:
      labels = labels[:actual_samples]
      benign_count = int(np.sum(labels == 0))
      anomaly_count = int(np.sum(labels == 1))
      print(
          f"[2/3] Loaded {len(labels)} ground truth labels "
          f"({benign_count} benign [0], {anomaly_count} anomalies [1])"
      )
    else:
      print("[2/3] No labels loaded.")
  else:
    print("[2/3] Skipping labels (none specified).")

  # 4. Forward Inference
  print(f"[3/3] Evaluating forward pass in batches of {batch_size}...")
  all_mse_scores = []
  all_sse_scores = []
  start_time = time.perf_counter()
  with torch.no_grad():
    for i in range(0, actual_samples, batch_size):
      batch_x = data_tensor[i : i + batch_size]
      sse_scores, _ = model(batch_x)
      mse_scores = sse_scores / float(num_features)
      all_sse_scores.append(sse_scores)
      all_mse_scores.append(mse_scores)

  end_time = time.perf_counter()
  total_time = end_time - start_time

  sse_np = torch.cat(all_sse_scores, dim=0).numpy()
  mse_np = torch.cat(all_mse_scores, dim=0).numpy()
  throughput = actual_samples / total_time if total_time > 0 else 0.0

  if threshold is None:
    threshold = float(np.percentile(mse_np, 99.0))
    threshold_desc = f"{threshold:.6e} (auto: 99th percentile)"
  else:
    threshold_desc = f"{threshold:.6e} (user-specified)"

  predicted_anomalies = mse_np >= threshold
  num_predicted_anomalies = int(np.sum(predicted_anomalies))

  print("\n" + "=" * 80)
  print("  Reconstruction Loss & Anomaly Detection Summary")
  print("=" * 80)
  print(f"Total Packets Evaluated:     {actual_samples}")
  print(f"Average Raw SSE Score:       {np.mean(sse_np):.6e}")
  print(
      f"Average Anomaly MSE Score:   {np.mean(mse_np):.6e}  (SSE /"
      f" {num_features} features)"
  )
  print(f"Min Anomaly MSE Score:       {np.min(mse_np):.6e}")
  print(f"Max Anomaly MSE Score:       {np.max(mse_np):.6e}")
  print(f"Std Dev Anomaly MSE Score:   {np.std(mse_np):.6e}")

  print("\n--- Percentile Distribution of Anomaly MSE Scores ---")
  print(f"  •  50th Percentile (Median): {np.percentile(mse_np, 50):.6e}")
  print(f"  •  90th Percentile:          {np.percentile(mse_np, 90):.6e}")
  print(f"  •  95th Percentile:          {np.percentile(mse_np, 95):.6e}")
  print(f"  •  99th Percentile:          {np.percentile(mse_np, 99):.6e}")
  print(f"  •  99.9th Percentile:        {np.percentile(mse_np, 99.9):.6e}")

  print(f"\n--- Anomaly Classification (Threshold: {threshold_desc}) ---")
  print(
      f"Packets Flagged as Anomaly:  {num_predicted_anomalies} /"
      f" {actual_samples}"
      f" ({num_predicted_anomalies / actual_samples * 100.0:.2f}%)"
  )
  print(
      f"Packets Flagged as Benign:   {actual_samples - num_predicted_anomalies}"
      f" / {actual_samples} "
      f"({(actual_samples - num_predicted_anomalies) / actual_samples * 100.0:.2f}%)"
  )

  # 5. Confusion Matrix Validation
  if labels is not None and len(labels) == actual_samples:
    print("\n" + "=" * 80)
    print("  Ground Truth Label Validation & Confusion Matrix")
    print("=" * 80)
    y_true = labels
    y_pred = predicted_anomalies.astype(int)

    tp = int(np.sum((y_true == 1) & (y_pred == 1)))
    tn = int(np.sum((y_true == 0) & (y_pred == 0)))
    fp = int(np.sum((y_true == 0) & (y_pred == 1)))
    fn = int(np.sum((y_true == 1) & (y_pred == 0)))

    accuracy = (tp + tn) / actual_samples * 100.0
    specificity = (tn / (tn + fp) * 100.0) if (tn + fp) > 0 else 0.0
    fpr = (fp / (fp + tn) * 100.0) if (fp + tn) > 0 else 0.0

    print(f"  • True Positives  (TP - Attack correctly detected):     {tp:6d}")
    print(f"  • True Negatives  (TN - Benign correctly classified):   {tn:6d}")
    print(f"  • False Positives (FP - Benign flagged as anomaly):     {fp:6d}")
    print(f"  • False Negatives (FN - Attack missed):                 {fn:6d}")
    print("  --------------------------------------------------")
    print(
        "  • Overall Classification Accuracy:                     "
        f" {accuracy:.2f}%"
    )
    print(
        "  • Benign Specificity (True Negative Rate):             "
        f" {specificity:.2f}%"
    )
    print(
        f"  • False Alarm Rate   (False Positive Rate):             {fpr:.2f}%"
    )
    if np.sum(y_true == 1) > 0:
      recall = (tp / (tp + fn) * 100.0) if (tp + fn) > 0 else 0.0
      precision = (tp / (tp + fp) * 100.0) if (tp + fp) > 0 else 0.0
      f1 = (
          (2 * precision * recall / (precision + recall))
          if (precision + recall) > 0
          else 0.0
      )
      print(
          "  • Detection Recall:                                    "
          f" {recall:.2f}%"
      )
      print(
          "  • Precision:                                           "
          f" {precision:.2f}%"
      )
      print(
          f"  • F1 Score:                                             {f1:.4f}"
      )
    else:
      print(
          "  [Note: Dataset segment evaluated consists entirely of benign"
          " baseline traffic (label=0)]"
      )

  print("\n--- Inference Performance ---")
  print(f"Total Inference Time:        {total_time:.4f} s")
  print(f"Cleartext Throughput:        {throughput:.2f} samples/sec")
  print(
      f"Average Inference Latency:   {total_time / actual_samples * 1000.0:.4f}"
      " ms / sample"
  )
  print("=" * 80)


def main():
  parser = argparse.ArgumentParser(
      description="Evaluate PyTorch KitNET over dataset suite."
  )
  parser.add_argument(
      "--num_samples",
      type=int,
      default=32768,
      help="Number of samples to evaluate",
  )
  parser.add_argument(
      "--num_features",
      type=int,
      default=5,
      help="Number of features per sample",
  )
  parser.add_argument("--batch_size", type=int, default=64, help="Batch size")
  parser.add_argument(
      "--threshold",
      type=float,
      default=None,
      help="Anomaly detection MSE threshold",
  )
  parser.add_argument(
      "--model_path",
      type=str,
      default="demos/network_anomaly/data/torch_kitnet_model.pt",
      help="Path to model checkpoint or binary file",
  )
  parser.add_argument(
      "--data_file",
      type=str,
      default="demos/network_anomaly/data/Mirai_first_batch_32K.bin",
      help="Path to binary dataset file",
  )
  parser.add_argument(
      "--labels_file",
      type=str,
      default="demos/network_anomaly/data/Mirai_labels.csv",
      help="Path to ground truth labels CSV file",
  )
  args = parser.parse_args()

  try:
    evaluate_suite(
        model_path=args.model_path,
        data_file=args.data_file,
        labels_file=args.labels_file,
        num_samples=args.num_samples,
        num_features=args.num_features,
        batch_size=args.batch_size,
        threshold=args.threshold,
    )
  except Exception as e:  # pylint: disable=broad-except
    print(f"Error: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  main()
