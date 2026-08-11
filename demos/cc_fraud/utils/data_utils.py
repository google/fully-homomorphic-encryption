"""Data utilities for credit card fraud demo."""

import numpy as np
import pandas as pd


def load_test_row(csv_path, row_idx=0):
  df = pd.read_csv(csv_path)
  row = df.iloc[row_idx]
  label = int(row["is_fraud"])
  features = row.drop("is_fraud").values.astype(np.float32).tolist()
  return features, label


def load_all_test_rows(csv_path):
  df = pd.read_csv(csv_path)
  labels = df["is_fraud"].values.astype(int).tolist()
  features_df = df.drop(columns=["is_fraud"])
  all_features = features_df.values.astype(np.float32).tolist()
  return all_features, labels
