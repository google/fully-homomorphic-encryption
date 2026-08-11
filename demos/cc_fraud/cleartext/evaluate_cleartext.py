"""Inference and validation script to measure activation ranges for MLPSigmoid."""

import os
import pickle

import numpy as np
import pandas as pd
from sklearn.metrics import accuracy_score
from sklearn.metrics import f1_score
from sklearn.metrics import roc_auc_score
from sklearn.model_selection import train_test_split
import torch
from torch import nn

from demos.cc_fraud.torch.model import MLPSigmoid
from demos.common.python import path_utils

resolve_path = path_utils.resolve_path


def main():
  if "BUILD_WORKSPACE_DIRECTORY" in os.environ:
    os.chdir(os.environ["BUILD_WORKSPACE_DIRECTORY"])

  checkpoint_path = resolve_path(
      "demos/cc_fraud/data/mlp_fraud_model_sigmoid.pt"
  )
  feature_cols_path = resolve_path(
      "demos/cc_fraud/data/feature_cols.pkl"
  )

  print(f"Loading checkpoint from {checkpoint_path}...")
  checkpoint = torch.load(
      checkpoint_path, map_location="cpu", weights_only=False
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

  # Load data
  feature_cols = pickle.load(open(feature_cols_path, "rb"))
  try:
    print("Loading test data from parquet...")
    # The dataset sparkov_fraud_encoded.parquet is not included in the repository.
    # See the README.md in the parent directory for instructions on how to download
    # and preprocess the Kaggle Credit Card Fraud Detection dataset.
    parquet_path = resolve_path(
        "demos/cc_fraud/data/sparkov_fraud_encoded.parquet"
    )
    df = pd.read_parquet(parquet_path)
    X = df[feature_cols].values.astype(np.float32)
    y = df["is_fraud"].values.astype(np.int64)
    # Retrieve the test split (same random seed/stratify as train.py)
    _, X_test, _, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y
    )
  except (FileNotFoundError, Exception) as e:  # pylint: disable=broad-except
    print(f"Failed to load parquet: {e}")
    print("Falling back to test_rows.csv...")
    csv_path = resolve_path(
        "demos/cc_fraud/data/test_rows.csv"
    )
    df = pd.read_csv(csv_path)
    y_test = df["is_fraud"].values.astype(np.int64)
    X_test = df[feature_cols].values.astype(np.float32)

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

  acc = accuracy_score(y_test, preds)
  f1 = f1_score(y_test, preds)
  auc = roc_auc_score(y_test, probs[:, 1])

  print("\nValidation Metrics:")
  print(f"  Accuracy: {acc:.6f}")
  print(f"  F1 Score: {f1:.6f}")
  print(f"  ROC AUC:  {auc:.6f}")

  # Process and print activation ranges from the hooks
  print("\nActivation Ranges (outputs of Linear / inputs to Sigmoid):")
  for name, output in activation_outputs.items():
    flat = output.flatten().numpy()
    mn = flat.min().item()
    mx = flat.max().item()
    p01 = np.percentile(flat, 1).item()
    p99 = np.percentile(flat, 99).item()
    p001 = np.percentile(flat, 0.1).item()
    p999 = np.percentile(flat, 99.9).item()

    print(f"Layer {name}:")
    print(f"  Absolute range:                 [{mn:.4f}, {mx:.4f}]")
    print(f"  98% range (0.01 to 0.99):       [{p01:.4f}, {p99:.4f}]")
    print(f"  99.8% range (0.001 to 0.999):   [{p001:.4f}, {p999:.4f}]")

  # Remove hooks
  for h in hooks:
    h.remove()


if __name__ == "__main__":
  main()
