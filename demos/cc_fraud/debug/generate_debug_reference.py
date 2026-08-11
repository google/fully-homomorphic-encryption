"""Generate debug reference data for the cc_fraud FHE demo."""

import json
import os
import sys

import numpy as np
import pandas as pd
import torch

from demos.cc_fraud.torch.model import MLPSigmoid
from demos.common.python import path_utils

resolve_path = path_utils.resolve_path


def main():
  csv_path = resolve_path(
      "demos/cc_fraud/data/test_rows.csv"
  )
  model_path = resolve_path(
      "demos/cc_fraud/data/mlp_fraud_model_sigmoid.pt"
  )
  output_path = resolve_path(
      "demos/cc_fraud/debug/debug_reference.json"
  )

  if not os.path.exists(model_path):
    print(f"Error: model file not found at {model_path}", file=sys.stderr)
    sys.exit(1)
  if not os.path.exists(csv_path):
    print(f"Error: CSV file not found at {csv_path}", file=sys.stderr)
    sys.exit(1)

  print(f"Loading model from {model_path}...")
  model = MLPSigmoid(input_dim=82, hidden_dims=[128, 64], num_classes=2)
  checkpoint = torch.load(model_path)
  model.load_state_dict(checkpoint["model_state_dict"])
  model.eval()

  print(f"Loading test rows from {csv_path}...")
  df = pd.read_csv(csv_path)
  features_df = df.drop(columns=["is_fraud"])
  all_features = features_df.values.astype(np.float32)

  reference_data = {}

  # Access layers from sequential net
  linear1 = model.net[0]
  sigmoid1 = model.net[1]
  linear2 = model.net[2]
  sigmoid2 = model.net[3]
  linear3 = model.net[4]

  print("Generating cleartext intermediate values...")
  with torch.no_grad():
    for idx in range(len(all_features)):
      x = torch.tensor(all_features[idx]).unsqueeze(0)  # shape [1, 82]

      row_ref = {}

      # Point 1: input
      row_ref["input"] = x.squeeze(0).tolist()

      # Layer 1 MatMul (before bias)
      matmul1 = torch.matmul(x, linear1.weight.t())
      row_ref["layer1_matmul"] = matmul1.squeeze(0).tolist()

      # Layer 1 Bias (Linear 1 output)
      x = linear1(x)
      row_ref["layer1_bias"] = x.squeeze(0).tolist()

      # Layer 1 Sigmoid
      x = sigmoid1(x)
      row_ref["layer1_sigmoid"] = x.squeeze(0).tolist()

      # Layer 2 MatMul
      matmul2 = torch.matmul(x, linear2.weight.t())
      row_ref["layer2_matmul"] = matmul2.squeeze(0).tolist()

      # Layer 2 Bias (Linear 2 output)
      x = linear2(x)
      row_ref["layer2_bias"] = x.squeeze(0).tolist()

      # Layer 2 Sigmoid
      x = sigmoid2(x)
      row_ref["layer2_sigmoid"] = x.squeeze(0).tolist()

      # Layer 3 MatMul
      matmul3 = torch.matmul(x, linear3.weight.t())
      row_ref["layer3_matmul"] = matmul3.squeeze(0).tolist()

      # Layer 3 Bias (Linear 3 output / logits)
      x = linear3(x)
      row_ref["layer3_bias"] = x.squeeze(0).tolist()

      reference_data[f"row_{idx}"] = row_ref

  print(f"Saving reference data to {output_path}...")
  os.makedirs(os.path.dirname(output_path), exist_ok=True)
  with open(output_path, "w") as f:
    json.dump(reference_data, f, indent=2)
  print("Done!")


if __name__ == "__main__":
  main()
