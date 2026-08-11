"""Exports PyTorch KitNET models to MLIR using shared export utilities."""

import argparse

import torch

from demos.common.python.export_mlir_utils import export_and_postprocess
from demos.common.python.path_utils import resolve_path
from demos.network_anomaly.torch.pytorch_50_kitnet import PyTorch50KitNET
from demos.network_anomaly.torch.pytorch_kitnet import PyTorchKitNET


def export_model(model_type: str, model_path: str, output_path: str) -> None:
  """Exports 5-feature or 50-feature KitNET model to annotated MLIR."""
  resolved_model_path = resolve_path(model_path)
  print(f"Loading {model_type} KitNET model from {resolved_model_path}...")

  if model_type == "50":
    num_features = 50
    if resolved_model_path.endswith(".bin"):
      model = PyTorch50KitNET.load_from_binary_model(resolved_model_path)
    else:
      model = PyTorch50KitNET.load_weights(resolved_model_path)
    entrypoint = "anomaly_50"
  else:
    num_features = 5
    if resolved_model_path.endswith(".bin"):
      model = PyTorchKitNET.load_from_binary_model(resolved_model_path)
    else:
      model = PyTorchKitNET.load_weights(resolved_model_path)
    entrypoint = "anomaly"

  model.eval()
  example_input = torch.randn(1, num_features, dtype=torch.float32)

  print(f"Exporting model to {output_path} with entrypoint '{entrypoint}'...")
  export_and_postprocess(
      model=model,
      example_input=example_input,
      output_file=output_path,
      entrypoint_name=entrypoint,
      use_fx=True,
  )
  print("✓ Export successfully completed.")


def main():
  parser = argparse.ArgumentParser(
      description="Export PyTorch KitNET models to MLIR."
  )
  parser.add_argument(
      "--model_type",
      type=str,
      choices=["5", "50"],
      default="5",
      help="Model feature size: '5' (MINI) or '50' (FULL)",
  )
  parser.add_argument(
      "--model_path",
      type=str,
      default="demos/network_anomaly/data/torch_kitnet_model.pt",
      help="Path to .pt checkpoint or .bin model file",
  )
  parser.add_argument(
      "--output",
      type=str,
      default="model_annotated.mlir",
      help="Output MLIR file path",
  )
  args = parser.parse_args()

  export_model(args.model_type, args.model_path, args.output)


if __name__ == "__main__":
  main()
