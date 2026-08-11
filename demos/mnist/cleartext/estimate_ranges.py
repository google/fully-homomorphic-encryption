"""Estimate activation ranges of MNIST cleartext model."""

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


def estimate_ranges(
    model_path: str,
    data_dir: str,
    num_samples: int,
    batch_size: int,
) -> None:
  """Runs evaluation and estimates activation ranges."""
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

  # Hooks to collect activations
  activation_inputs = {
      "fc1": [],
      "fc2": [],
  }

  def make_hook(name):
    def hook(module, input, output):
      # The input to ReLU is the output of the Linear layer (fc1/fc2)
      # We collect the output of the linear layer.
      activation_inputs[name].append(output.detach().cpu())

    return hook

  model.fc1.register_forward_hook(make_hook("fc1"))
  model.fc2.register_forward_hook(make_hook("fc2"))

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
  print(
      f"Evaluated {actual_samples} samples in {total_time:.4f} s. Accuracy:"
      f" {accuracy:.2f}%"
  )

  print("\n--- Activation Input Range Analysis ---")
  ranges = {}
  for layer_name, tensors in activation_inputs.items():
    all_outputs = torch.cat(tensors, dim=0)
    # Flatten to get global statistics for the layer
    flat_outputs = all_outputs.view(-1).numpy()

    abs_outputs = np.abs(flat_outputs)

    print(f"\nLayer: {layer_name} (shape: {all_outputs.shape})")
    print(f"  Actual Min: {np.min(flat_outputs):.6f}")
    print(f"  Actual Max: {np.max(flat_outputs):.6f}")

    # Using 99%ile asymmetric bounds as default for annotation
    lower_99 = np.percentile(flat_outputs, 0.5)
    upper_99 = np.percentile(flat_outputs, 99.5)
    ranges[layer_name] = (lower_99, upper_99)

    print("  Asymmetric percentiles:")
    print(f"    0.5%ile (lower 99%): {lower_99:.6f}")
    print(f"    2.5%ile (lower 95%): {np.percentile(flat_outputs, 2.5):.6f}")
    print(f"    12.5%ile (lower 75%): {np.percentile(flat_outputs, 12.5):.6f}")
    print(f"    87.5%ile (upper 75%): {np.percentile(flat_outputs, 87.5):.6f}")
    print(f"    97.5%ile (upper 95%): {np.percentile(flat_outputs, 97.5):.6f}")
    print(f"    99.5%ile (upper 99%): {upper_99:.6f}")

    print("  Symmetric bounds (based on absolute values):")
    print(
        f"    75%ile: [-{np.percentile(abs_outputs, 75):.6f},"
        f" {np.percentile(abs_outputs, 75):.6f}]"
    )
    print(
        f"    95%ile: [-{np.percentile(abs_outputs, 95):.6f},"
        f" {np.percentile(abs_outputs, 95):.6f}]"
    )
    print(
        f"    99%ile: [-{np.percentile(abs_outputs, 99):.6f},"
        f" {np.percentile(abs_outputs, 99):.6f}]"
    )
    print(
        f"    Max:    [-{np.max(abs_outputs):.6f}, {np.max(abs_outputs):.6f}]"
    )


def main():
  parser = argparse.ArgumentParser(
      description="Estimate activation ranges of MNIST cleartext model."
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
    estimate_ranges(
        args.model_path,
        args.data_dir,
        args.num_samples,
        args.batch_size,
    )
  except Exception as e:
    print(f"Error executing estimation: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  main()
