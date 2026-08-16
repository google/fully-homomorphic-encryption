"""Estimate activation ranges of credit card fraud cleartext model."""

import argparse
import os
import pickle
import sys

import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
import torch
from torch import nn

from demos.cc_fraud.torch.model import MLPSigmoid
from demos.common.python import path_utils

resolve_path = path_utils.resolve_path


def estimate_ranges(
    model_path: str,
    feature_cols_path: str,
    data_path: str,
) -> None:
  """Runs evaluation and estimates activation ranges."""
  resolved_model_path = resolve_path(model_path)
  if not os.path.exists(resolved_model_path):
    raise FileNotFoundError(
        f"Model file not found at {model_path} (resolved:"
        f" {resolved_model_path})"
    )

  print(f"Loading checkpoint from {resolved_model_path}...")
  checkpoint = torch.load(
      resolved_model_path, map_location="cpu", weights_only=False
  )

  input_dim = checkpoint["input_dim"]
  hidden_dims = checkpoint["hidden_dims"]
  num_classes = checkpoint["n_classes"]

  print(
      f"Model parameters: input_dim={input_dim}, hidden_dims={hidden_dims},"
      f" num_classes={num_classes}"
  )

  # Instantiate model and load state dict
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

  # Load data
  resolved_data_path = resolve_path(data_path)
  if not os.path.exists(resolved_data_path):
    raise FileNotFoundError(
        f"Data file not found at {data_path} (resolved:"
        f" {resolved_data_path})"
    )

  print(f"Loading test data from {resolved_data_path}...")
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

  X_test_tensor = torch.tensor(X_test)

  # Register forward hooks on Linear layers (except the final output layer) to
  # capture inputs to Sigmoid
  activation_outputs = {}

  def make_hook(name):
    # pylint: disable=unused-argument
    def hook(module, input_tensor, output):
      activation_outputs[name] = output.detach()

    # pylint: enable=unused-argument

    return hook

  hooks = []
  linear_count = 0
  for name, module in model.named_modules():
    if isinstance(module, nn.Linear) and module is not model.net[-1]:
      hook_name = f"Linear_{linear_count}"
      hooks.append(module.register_forward_hook(make_hook(hook_name)))
      linear_count += 1

  print(f"Registered {len(hooks)} hooks on Linear layers.")

  # Run evaluation
  print("Running validation on the test set...")
  with torch.no_grad():
    logits = model(X_test_tensor)
    probs = torch.softmax(logits, dim=1).numpy()
    preds = np.argmax(probs, axis=1)

  from sklearn.metrics import accuracy_score
  from sklearn.metrics import f1_score
  from sklearn.metrics import roc_auc_score

  acc = accuracy_score(y_test, preds)
  f1 = f1_score(y_test, preds)
  auc = roc_auc_score(y_test, probs[:, 1])

  print("\nValidation Metrics:")
  print(f"  Accuracy: {acc:.6f}")
  print(f"  F1 Score: {f1:.6f}")
  print(f"  ROC AUC:  {auc:.6f}")

  # Process and print activation ranges from the hooks
  print("\n--- Activation Range Analysis ---")
  print("Activation Ranges (outputs of Linear / inputs to Sigmoid):")
  for name, output in activation_outputs.items():
    flat = output.flatten().numpy()
    mn = flat.min().item()
    mx = flat.max().item()
    p01 = np.percentile(flat, 1).item()
    p99 = np.percentile(flat, 99).item()
    p001 = np.percentile(flat, 0.1).item()
    p999 = np.percentile(flat, 99.9).item()

    print(f"\nLayer {name}:")
    print(f"  Absolute range:                 [{mn:.4f}, {mx:.4f}]")
    print(f"  98% range (0.01 to 0.99):       [{p01:.4f}, {p99:.4f}]")
    print(f"  99.8% range (0.001 to 0.999):   [{p001:.4f}, {p999:.4f}]")

  # Remove hooks
  for h in hooks:
    h.remove()


def main():
  parser = argparse.ArgumentParser(
      description="Estimate activation ranges of credit card fraud cleartext"
      " model."
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
    estimate_ranges(
        args.model_path,
        args.feature_cols_path,
        args.data_path,
    )
  except Exception as e:
    print(f"Error executing estimation: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  main()
