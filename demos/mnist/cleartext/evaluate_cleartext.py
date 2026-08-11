"""Cleartext evaluation runner for a single MNIST sample using PyTorch."""

import argparse
import os
import sys
import time

import numpy as np
import torch

from demos.common.python import path_utils
from demos.mnist.torch.model import CanonicalMLP
from demos.mnist.utils.mnist_data import MnistDataset

resolve_path = path_utils.resolve_path


def load_mnist_sample(data_dir: str, sample_idx: int) -> tuple[np.ndarray, int]:
  """Loads and normalizes a single image and label from MNIST NPZ file."""
  npz_path = resolve_path(os.path.join(data_dir, "mnist.npz"))
  dataset = MnistDataset(npz_path, reshape_to_2d=False)
  image, label = dataset[sample_idx]
  image = image.reshape(1, 1, 28, 28)
  return image, label


def evaluate_single(model_path: str, data_dir: str, sample_idx: int) -> None:
  """Loads the model and evaluates a single MNIST sample."""
  resolved_model_path = resolve_path(model_path)
  if not os.path.exists(resolved_model_path):
    raise FileNotFoundError(
        f"Model file not found at {model_path} (resolved:"
        f" {resolved_model_path})"
    )

  print(f"Loading model from: {resolved_model_path}")
  model = CanonicalMLP()
  model.load_state_dict(torch.load(resolved_model_path, map_location="cpu"))
  model.eval()

  image, label = load_mnist_sample(data_dir, sample_idx)
  input_tensor = torch.from_numpy(image)

  start_time = time.perf_counter()
  with torch.no_grad():
    output = model(input_tensor)
  end_time = time.perf_counter()

  latency_ms = (end_time - start_time) * 1000.0
  pred = int(torch.argmax(output, dim=1).item())
  is_correct = pred == label

  print(f"\nEvaluating MNIST Sample Index: {sample_idx}")
  print(f"True Label:      {label}")
  print(f"Predicted Label: {pred}")
  print(f"Result:          {'CORRECT' if is_correct else 'INCORRECT'}")
  print(f"Latency:         {latency_ms:.4f} ms")


def main():
  parser = argparse.ArgumentParser(
      description="Evaluate cleartext MNIST model on a single sample."
  )
  parser.add_argument(
      "--sample_idx",
      type=int,
      default=0,
      help="Index of the MNIST sample to evaluate (0-9999)",
  )
  parser.add_argument(
      "--model_path",
      type=str,
      default="demos/mnist/data/mlp_model.pth",
      help="Path to PyTorch model checkpoint file",
  )
  parser.add_argument(
      "--data_dir",
      type=str,
      default=(
          "demos/mnist/data"
      ),
      help="Directory containing MNIST dataset binary files",
  )
  args = parser.parse_args()

  try:
    evaluate_single(args.model_path, args.data_dir, args.sample_idx)
  except Exception as e:
    print(f"Error evaluating sample {args.sample_idx}: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  main()
