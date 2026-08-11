"""Convert a PyTorch CanonicalMLP model to MLIR."""

import argparse
import os
import sys

import torch

from demos.common.python import export_mlir_utils
from demos.common.python import path_utils
from demos.mnist.torch.model import CanonicalMLP

resolve_path = path_utils.resolve_path


def main():
  parser = argparse.ArgumentParser(
      description="Convert a PyTorch model to MLIR."
  )
  parser.add_argument(
      "--input_model",
      default="demos/mnist/data/mlp_model.pth",
      help="Path to the input PyTorch model file",
  )
  parser.add_argument(
      "--output_file",
      default=(
          "demos/mnist/data/mnist.mlir"
      ),
      help="Path where the resulting .mlir file will be saved",
  )

  args = parser.parse_args()

  input_path = resolve_path(args.input_model)
  if not os.path.exists(input_path):
    print(
        f"Error: input model not found at {args.input_model} (resolved:"
        f" {input_path})",
        file=sys.stderr,
    )
    sys.exit(1)

  model = CanonicalMLP()
  try:
    model.load_state_dict(torch.load(input_path, map_location="cpu"))
  except Exception as e:
    print(f"Error loading model from {input_path}: {e}", file=sys.stderr)
    sys.exit(1)

  model.eval()

  example_input = torch.randn(1, 1, 28, 28)

  # Use common utility for export and post-processing
  export_mlir_utils.export_and_postprocess(
      model,
      example_input,
      args.output_file,
      entrypoint_name="mnist",
      use_fx=True,
  )


if __name__ == "__main__":
  main()
