"""Train the fraud detection MLP with Sigmoid activations.

Architecture:  Linear -> Sigmoid (x2), then Linear.
"""

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
from torch.utils.data import DataLoader
from torch.utils.data import TensorDataset

from demos.cc_fraud.torch.model import MLPSigmoid
from demos.common.python import path_utils

# ---------------------------------------------------------------------------
# Pre-activation penalty: hinge-squared loss outside [-bound, bound]
# ---------------------------------------------------------------------------
def preact_penalty(preacts, bound=6.0):
  """Compute mean squared excess beyond ±bound.

  discourages pre-activations from escaping the Chebyshev domain.
  """
  total = 0.0
  for pa in preacts:
    excess = torch.clamp(pa.abs() - bound, min=0.0)
    total = total + (excess**2).mean()
  return total


# ---------------------------------------------------------------------------
# Training loop
# ---------------------------------------------------------------------------
def train_model(
    model,
    train_loader,
    epochs=100,
    lr=1e-3,
    preact_bound=6.0,
    preact_lambda=0.10,
):
  optimizer = torch.optim.Adam(model.parameters(), lr=lr)
  criterion = nn.CrossEntropyLoss()
  use_preact_reg = preact_lambda > 0 and hasattr(model, "forward_with_preacts")
  model.train()
  for epoch in range(epochs):
    total_loss = 0
    total_penalty = 0
    for X_batch, y_batch in train_loader:
      optimizer.zero_grad()
      if use_preact_reg:
        logits, preacts = model.forward_with_preacts(X_batch)
        ce_loss = criterion(logits, y_batch)
        pa_loss = preact_penalty(preacts, bound=preact_bound)
        loss = ce_loss + preact_lambda * pa_loss
        total_penalty += pa_loss.item() * X_batch.size(0)
      else:
        logits = model(X_batch)
        loss = criterion(logits, y_batch)
      loss.backward()
      optimizer.step()
      total_loss += loss.item() * X_batch.size(0)
    if (epoch + 1) % 20 == 0:
      avg = total_loss / len(train_loader.dataset)
      msg = f"  Epoch {epoch+1:3d}/{epochs}  loss={avg:.6f}"
      if use_preact_reg:
        avg_pa = total_penalty / len(train_loader.dataset)
        msg += f"  preact_penalty={avg_pa:.4f}"
      print(msg)
  model.eval()
  return model


def evaluate(model, X_tensor, y_np, label="Model"):
  with torch.no_grad():
    logits = model(X_tensor)
    probs = torch.softmax(logits, dim=1).numpy()
    preds = np.argmax(probs, axis=1)

  acc = accuracy_score(y_np, preds)
  f1 = f1_score(y_np, preds)
  auc = roc_auc_score(y_np, probs[:, 1])
  print(f"  {label:20s}  accuracy={acc:.4f}  F1={f1:.4f}  AUC={auc:.4f}")
  return acc, f1, auc


def main():
  print("Loading data...")

  feature_cols_path = path_utils.resolve_path(
      "demos/cc_fraud/data/feature_cols.pkl"
  )
  encoded_data_path = path_utils.resolve_path(
      "demos/cc_fraud/data/sparkov_fraud_encoded.parquet"
  )
  save_path = path_utils.resolve_path(
      "demos/cc_fraud/data/mlp_fraud_model_sigmoid.pt"
  )

  feature_cols = pickle.load(open(feature_cols_path, "rb"))
  # The dataset sparkov_fraud_encoded.parquet is not included in the repository.
  # See the README.md in the parent directory for instructions on how to
  # download and preprocess the Kaggle Credit Card Fraud Detection dataset.
  df = pd.read_parquet(encoded_data_path)
  X = df[feature_cols].values.astype(np.float32)
  y = df["is_fraud"].values.astype(np.int64)
  print(f"  Shape: {X.shape}, fraud rate: {y.mean():.4%}")

  X_train, X_test, y_train, y_test = train_test_split(
      X, y, test_size=0.2, random_state=42, stratify=y
  )

  X_train_t = torch.tensor(X_train)
  y_train_t = torch.tensor(y_train)
  X_test_t = torch.tensor(X_test)

  train_ds = TensorDataset(X_train_t, y_train_t)
  train_loader = DataLoader(train_ds, batch_size=1024, shuffle=True)

  input_dim = X.shape[1]
  hidden_dims = [128, 64]
  num_classes = 2

  PREACT_BOUND = 6.0  # target: pre-activations within [-6, 6]
  PREACT_LAMBDA = 0.10  # regularization weight

  print(
      f"\nTraining Sigmoid MLP (preact bound=±{PREACT_BOUND},"
      f" lambda={PREACT_LAMBDA})..."
  )
  model = MLPSigmoid(input_dim, hidden_dims, num_classes)
  model = train_model(
      model,
      train_loader,
      epochs=100,
      lr=1e-3,
      preact_bound=PREACT_BOUND,
      preact_lambda=PREACT_LAMBDA,
  )

  print("\nTest set results:")
  evaluate(model, X_test_t, y_test, label="Sigmoid MLP")

  torch.save(
      {
          "model_state_dict": model.state_dict(),
          "input_dim": input_dim,
          "n_classes": num_classes,
          "activation": "sigmoid",
          "batch_norm": False,
          "hidden_dims": hidden_dims,
      },
      save_path,
  )
  print(f"\nSaved Sigmoid model to {save_path}")


if __name__ == "__main__":
  main()
