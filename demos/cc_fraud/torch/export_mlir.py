"""Export the MLP to MLIR (linalg-on-tensors) using torch-mlir."""

import argparse
import sys

import torch

from demos.cc_fraud.torch.model import MLPSigmoid
from demos.common.python import export_mlir_utils
from demos.common.python import path_utils


def main():
  parser = argparse.ArgumentParser(description="Export CC Fraud model to MLIR.")
  parser.add_argument(
      "--input_model",
      default="demos/cc_fraud/data/mlp_fraud_model_sigmoid.pt",
      help="Path to the input PyTorch model file",
  )
  parser.add_argument(
      "--output_file",
      default=(
          "demos/cc_fraud/data/model_annotated.mlir"
      ),
      help="Path where the resulting .mlir file will be saved",
  )
  args = parser.parse_args()

  input_path = path_utils.resolve_path(args.input_model)
  output_path = path_utils.resolve_path(args.output_file)

  print(f"Loading model from {input_path}...")
  try:
    checkpoint = torch.load(input_path, map_location="cpu", weights_only=False)
  except Exception as e:
    print(f"Error loading model from {input_path}: {e}", file=sys.stderr)
    sys.exit(1)

  input_dim = checkpoint["input_dim"]
  n_classes = checkpoint["n_classes"]
  hidden_dims = checkpoint["hidden_dims"]

  model = MLPSigmoid(
      input_dim=input_dim,
      hidden_dims=hidden_dims,
      num_classes=n_classes,
  )
  model.load_state_dict(checkpoint["model_state_dict"])

  # Sample input for export (batch size 1)
  sample_input = torch.randn(1, input_dim)

  # Export to MLIR using common utility
  # We rename the entrypoint to 'cc_fraud'
  export_mlir_utils.export_and_postprocess(
      model,
      sample_input,
      output_path,
      entrypoint_name="cc_fraud",
      use_fx=True,
  )


if __name__ == "__main__":
  main()
