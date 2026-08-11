"""Single-sample cleartext evaluation runner for PyTorch KitNET."""

import argparse
import os
import sys
import time

import numpy as np
import torch

from demos.common.python import path_utils
from demos.network_anomaly.torch import pytorch_kitnet

PyTorchKitNET = pytorch_kitnet.PyTorchKitNET
resolve_path = path_utils.resolve_path


def evaluate_single(
    sample_idx: int = 0,
    model_path: str = "demos/network_anomaly/data/torch_kitnet_model.pt",
    data_file: str = "demos/network_anomaly/data/Mirai_first_batch_32K.bin",
    num_features: int = 5,
    verbose: bool = True,
) -> None:
  """Evaluates a single packet vector from binary dataset using PyTorch KitNET."""
  resolved_model = resolve_path(model_path)
  resolved_data = resolve_path(data_file)

  print("=" * 80)
  print("  PyTorch KitNET Cleartext Single Packet Evaluation")
  print("=" * 80)
  print(f"Model Checkpoint:   {resolved_model}")
  print(f"Dataset File:       {resolved_data}")
  print(f"Sample Index:       {sample_idx}")
  print(f"Number of Features: {num_features}\n")

  # 1. Load Model
  if resolved_model.endswith(".bin"):
    model = PyTorchKitNET.load_from_binary_model(resolved_model)
  else:
    model = PyTorchKitNET.load_weights(resolved_model)
  model.eval()

  # 2. Read Single Sample from Binary File
  bytes_per_sample = num_features * 8
  offset = sample_idx * bytes_per_sample

  if not os.path.exists(resolved_data):
    raise FileNotFoundError(f"Dataset file not found at {resolved_data}")

  with open(resolved_data, "rb") as f:
    f.seek(offset)
    raw_bytes = f.read(bytes_per_sample)
    if len(raw_bytes) < bytes_per_sample:
      raise ValueError(
          f"Incomplete sample read: expected {bytes_per_sample} bytes, got"
          f" {len(raw_bytes)}"
      )
    features_np = np.frombuffer(raw_bytes, dtype=np.float64).astype(np.float32)

  x = torch.from_numpy(features_np).unsqueeze(0)

  # 3. Forward Inference
  t0 = time.perf_counter()
  with torch.no_grad():
    sse_score, reconstructed = model(x)
    mse_score = sse_score.item() / float(num_features)
    latency_ms = (time.perf_counter() - t0) * 1000.0

  print("--- Cleartext Evaluation Single Sample Results ---")
  print(f"Sample Index:            {sample_idx}")
  print(f"Raw SSE Score:           {sse_score.item():.6e}")
  print(
      f"Reconstruction MSE:      {mse_score:.6e}  (SSE / {num_features}"
      " features)"
  )
  print(f"Inference Latency:       {latency_ms:.4f} ms")

  if verbose:
    print(f"\nInput Features:          {features_np.tolist()}")
    print(f"Reconstructed Features:  {reconstructed.squeeze(0).tolist()}")
  print("=" * 80)


def main():
  parser = argparse.ArgumentParser(
      description="Evaluate PyTorch KitNET on a single packet sample."
  )
  parser.add_argument(
      "--sample_idx", type=int, default=0, help="Index of the packet sample"
  )
  parser.add_argument(
      "--model_path",
      type=str,
      default="demos/network_anomaly/data/torch_kitnet_model.pt",
      help="Path to .pt checkpoint or .bin model file",
  )
  parser.add_argument(
      "--data_file",
      type=str,
      default="demos/network_anomaly/data/Mirai_first_batch_32K.bin",
      help="Path to binary packet dataset file",
  )
  parser.add_argument(
      "--num_features",
      type=int,
      default=5,
      help="Number of features per sample",
  )
  parser.add_argument(
      "--verbose",
      action="store_true",
      default=True,
      help="Print feature vectors",
  )
  args = parser.parse_args()

  try:
    evaluate_single(
        sample_idx=args.sample_idx,
        model_path=args.model_path,
        data_file=args.data_file,
        num_features=args.num_features,
        verbose=args.verbose,
    )
  except Exception as e:  # pylint: disable=broad-except
    print(f"Error: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  main()
