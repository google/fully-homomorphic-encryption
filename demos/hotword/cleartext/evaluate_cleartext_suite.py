# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Cleartext evaluation suite runner over Speech Commands test dataset using PyTorch."""

import argparse
import os
import sys
import time

import torch

from demos.common.python import path_utils
from demos.hotword.torch import model as tc_resnet
from demos.hotword.utils import hotword_data
from demos.hotword.utils.hotword_data import HotwordDataset

LABELS = hotword_data.LABELS
resolve_path = path_utils.resolve_path


def load_suite(
    npz_path: str, num_samples: int
) -> tuple[torch.Tensor, torch.Tensor]:
  """Loads up to num_samples images and labels from test_data.npz."""
  resolved_npz_path = resolve_path(npz_path)
  dataset = HotwordDataset(npz_path=resolved_npz_path)

  # HotwordDataset stores x and y when loading from NPZ
  x = dataset.x[:num_samples]
  y = dataset.y[:num_samples]
  return x, y


def evaluate_suite(
    model_path: str, npz_path: str, num_samples: int, batch_size: int
) -> None:
  """Evaluates cleartext model over batched dataset and reports accuracy/throughput."""
  resolved_model_path = resolve_path(model_path)
  if not os.path.exists(resolved_model_path):
    raise FileNotFoundError(
        f"Model file not found at {model_path} (resolved:"
        f" {resolved_model_path})"
    )

  print(f"Loading model from: {resolved_model_path}")

  # Try to detect model shape from checkpoint
  state = None
  n_mfcc = 10
  num_classes = len(LABELS)
  try:
    state = torch.load(resolved_model_path, map_location="cpu")
    if "fc.weight" in state:
      num_classes = state["fc.weight"].shape[0]
    if "stem_conv.weight" in state:
      n_mfcc = state["stem_conv.weight"].shape[1]
    elif "stem.0.weight" in state:
      n_mfcc = state["stem.0.weight"].shape[1]
    print(f"Detected model config: n_mfcc={n_mfcc}, num_classes={num_classes}")
  except Exception as e:
    print(f"Failed to read checkpoint header: {e}")
    print("Using default config (n_mfcc=10, num_classes=13)")

  model = tc_resnet.tc_resnet8(n_mfcc=n_mfcc, num_classes=num_classes)

  use_random_model = False
  if state is not None:
    try:
      model.load_state_dict(state, strict=False)
    except Exception as e:  # pylint: disable=broad-except
      print(f"Failed to load model: {e}")
      print("Using randomly initialized model for evaluation.")
      use_random_model = True
  else:
    print("Using randomly initialized model for evaluation.")
    use_random_model = True

  model.eval()

  print(f"Loading up to {num_samples} samples from {npz_path}...")
  try:
    x_tensor, y_tensor = load_suite(npz_path, num_samples)
    actual_samples = len(y_tensor)
  except Exception as e:  # pylint: disable=broad-except
    print(f"Failed to load suite: {e}")
    print("Generating dummy suite for evaluation.")
    actual_samples = min(num_samples, 100)
    x_tensor = torch.randn(actual_samples, n_mfcc, 48 if n_mfcc == 10 else 101)
    y_tensor = torch.zeros(actual_samples, dtype=torch.long)

  if actual_samples == 0:
    print("No samples loaded.")
    return

  correct = 0
  start_time = time.perf_counter()
  with torch.no_grad():
    for i in range(0, actual_samples, batch_size):
      batch_imgs = x_tensor[i : i + batch_size]
      batch_lbls = y_tensor[i : i + batch_size]
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
  if use_random_model:
    print(
        "WARNING: Used randomly initialized model. Accuracy is not meaningful."
    )


def main():
  parser = argparse.ArgumentParser(
      description="Evaluate cleartext Hotword model over test dataset."
  )
  parser.add_argument(
      "--num_samples",
      type=int,
      default=1000,
      help="Number of test samples to evaluate",
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
      default="demos/hotword/data/tcresnet8-small.pth",
      help="Path to PyTorch model checkpoint file",
  )
  parser.add_argument(
      "--npz_path",
      type=str,
      default="demos/hotword/data/test_data-small.npz",
      help="Path to test_data.npz",
  )
  args = parser.parse_args()

  try:
    evaluate_suite(
        args.model_path, args.npz_path, args.num_samples, args.batch_size
    )
  except Exception as e:
    print(f"Error executing evaluation suite: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  main()
