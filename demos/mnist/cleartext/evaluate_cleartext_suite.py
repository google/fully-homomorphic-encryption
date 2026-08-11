"""Cleartext evaluation suite runner over MNIST test dataset using PyTorch."""

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


def load_mnist_suite(
    data_dir: str, num_samples: int
) -> tuple[np.ndarray, np.ndarray]:
  """Loads up to num_samples images and labels from MNIST NPZ file."""
  npz_path = resolve_path(os.path.join(data_dir, "mnist.npz"))
  dataset = MnistDataset(npz_path, reshape_to_2d=True, max_samples=num_samples)
  return dataset.images, dataset.labels


def evaluate_suite(
    model_path: str, data_dir: str, num_samples: int, batch_size: int
) -> None:
  """Evaluates cleartext model over batched MNIST dataset and reports accuracy/throughput."""
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

  print(f"Loading up to {num_samples} samples from {data_dir}...")
  images, labels = load_mnist_suite(data_dir, num_samples)
  actual_samples = len(labels)
  if actual_samples == 0:
    print("No samples loaded.")
    return

  images_tensor = torch.from_numpy(images)
  labels_tensor = torch.from_numpy(labels)

  correct = 0
  start_time = time.perf_counter()
  with torch.no_grad():
    for i in range(0, actual_samples, batch_size):
      batch_imgs = images_tensor[i : i + batch_size]
      batch_lbls = labels_tensor[i : i + batch_size]
      outputs = model(batch_imgs)
      preds = torch.argmax(outputs, dim=1)
      correct += int((preds == batch_lbls).sum().item())
  end_time = time.perf_counter()

  total_time = end_time - start_time
  accuracy = (correct / actual_samples) * 100.0
  throughput = actual_samples / total_time if total_time > 0 else 0.0

  print("\n--- Evaluation Suite Results ---")
  print(f"Total Samples Evaluated: {actual_samples}")
  print(f"Batch Size:              {batch_size}")
  print(f"Total Correct:           {correct} / {actual_samples}")
  print(f"Accuracy:                {accuracy:.2f}%")
  print(f"Total Evaluation Time:   {total_time:.4f} s")
  print(f"Throughput:              {throughput:.2f} samples/sec")


def main():
  parser = argparse.ArgumentParser(
      description="Evaluate cleartext MNIST model over test dataset."
  )
  parser.add_argument(
      "--num_samples",
      type=int,
      default=10000,
      help="Number of test samples to evaluate (max 10000)",
  )
  parser.add_argument(
      "--batch_size",
      type=int,
      default=64,
      help="Batch size for evaluation",
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
    evaluate_suite(
        args.model_path, args.data_dir, args.num_samples, args.batch_size
    )
  except Exception as e:
    print(f"Error executing evaluation suite: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  main()
