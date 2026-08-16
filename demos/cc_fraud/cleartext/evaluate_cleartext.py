"""Cleartext evaluation runner for a single credit card fraud sample using PyTorch."""

import argparse
import os
import pickle
import sys
import time

import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
import torch

from demos.cc_fraud.torch.model import MLPSigmoid
from demos.common.python import path_utils

resolve_path = path_utils.resolve_path


def load_sample(
    data_path: str, feature_cols: list[str], sample_idx: int
) -> tuple[np.ndarray, int]:
  """Loads a single sample from the dataset."""
  resolved_data_path = resolve_path(data_path)
  if not os.path.exists(resolved_data_path):
    raise FileNotFoundError(
        f"Data file not found at {data_path} (resolved:"
        f" {resolved_data_path})"
    )

  if data_path.endswith(".parquet"):
    df = pd.read_parquet(resolved_data_path)
    X = df[feature_cols].values.astype(np.float32)
    y = df["is_fraud"].values.astype(np.int64)
    # Retrieve the test split (same random seed/stratify as train.py)
    _, X_test, _, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y
    )
  elif data_path.endswith(".csv"):
    df = pd.read_csv(resolved_data_path)
    y_test = df["is_fraud"].values.astype(np.int64)
    X_test = df[feature_cols].values.astype(np.float32)
  else:
    raise ValueError(
        f"Unsupported data format: {data_path}. Expected .parquet or .csv"
    )

  if sample_idx < 0 or sample_idx >= len(X_test):
    raise ValueError(
        f"Sample index {sample_idx} out of range [0, {len(X_test) - 1}]"
    )

  return X_test[sample_idx : sample_idx + 1], y_test[sample_idx]


def evaluate_single(
    model_path: str,
    feature_cols_path: str,
    data_path: str,
    sample_idx: int,
) -> None:
  """Loads the model and evaluates a single fraud detection sample."""
  resolved_model_path = resolve_path(model_path)
  if not os.path.exists(resolved_model_path):
    raise FileNotFoundError(
        f"Model file not found at {model_path} (resolved:"
        f" {resolved_model_path})"
    )

  print(f"Loading model from: {resolved_model_path}")
  checkpoint = torch.load(
      resolved_model_path, map_location="cpu", weights_only=False
  )

  input_dim = checkpoint["input_dim"]
  hidden_dims = checkpoint["hidden_dims"]
  num_classes = checkpoint["n_classes"]

  model = MLPSigmoid(input_dim, hidden_dims, num_classes)
  model.load_state_dict(checkpoint["model_state_dict"])
  model.eval()

  # Load feature columns
  resolved_feature_cols_path = resolve_path(feature_cols_path)
  if not os.path.exists(resolved_feature_cols_path):
    raise FileNotFoundError(
        f"Feature columns file not found at {feature_cols_path} (resolved:"
        f" {resolved_feature_cols_path})"
    )
  feature_cols = pickle.load(open(resolved_feature_cols_path, "rb"))

  # Load sample
  sample, label = load_sample(data_path, feature_cols, sample_idx)
  input_tensor = torch.from_numpy(sample)

  start_time = time.perf_counter()
  with torch.no_grad():
    logits = model(input_tensor)
    probs = torch.softmax(logits, dim=1)
  end_time = time.perf_counter()

  latency_ms = (end_time - start_time) * 1000.0
  pred = int(torch.argmax(probs, dim=1).item())
  fraud_prob = probs[0, 1].item()
  is_correct = pred == label

  print(f"\nEvaluating Credit Card Fraud Sample Index: {sample_idx}")
  print(f"True Label:      {label} ({'FRAUD' if label == 1 else 'LEGITIMATE'})")
  print(
      f"Predicted Label: {pred} ({'FRAUD' if pred == 1 else 'LEGITIMATE'})"
  )
  print(f"Fraud Probability: {fraud_prob:.6f}")
  print(f"Result:          {'CORRECT' if is_correct else 'INCORRECT'}")
  print(f"Latency:         {latency_ms:.4f} ms")


def main():
  parser = argparse.ArgumentParser(
      description="Evaluate cleartext credit card fraud model on a single"
      " sample."
  )
  parser.add_argument(
      "--sample_idx",
      type=int,
      default=0,
      help="Index of the sample to evaluate",
  )
  parser.add_argument(
      "--model_path",
      type=str,
      default="demos/cc_fraud/data/mlp_fraud_model_sigmoid.pt",
      help="Path to PyTorch model checkpoint file",
  )
  parser.add_argument(
      "--feature_cols_path",
      type=str,
      default="demos/cc_fraud/data/feature_cols.pkl",
      help="Path to feature columns pickle file",
  )
  parser.add_argument(
      "--data_path",
      type=str,
      default="demos/cc_fraud/data/test_rows.csv",
      help=(
          "Path to test data file (.csv or .parquet). For parquet files, the"
          " test split will be extracted; for CSV files, all rows are used."
      ),
  )
  args = parser.parse_args()

  try:
    evaluate_single(
        args.model_path,
        args.feature_cols_path,
        args.data_path,
        args.sample_idx,
    )
  except Exception as e:
    print(f"Error evaluating sample {args.sample_idx}: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  main()
