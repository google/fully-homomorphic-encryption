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

"""Cleartext evaluation runner for a single Speech Commands sample using PyTorch."""

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


def load_sample(npz_path: str, sample_idx: int) -> tuple[torch.Tensor, int]:
  """Loads a single sample from test_data.npz."""
  resolved_npz_path = resolve_path(npz_path)
  dataset = HotwordDataset(npz_path=resolved_npz_path)
  x, y = dataset[sample_idx]
  x = x.unsqueeze(0)
  return x, int(y.item())


def evaluate_single(model_path: str, npz_path: str, sample_idx: int) -> None:
  """Loads the model and evaluates a single sample."""
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

  if state is not None:
    try:
      model.load_state_dict(state, strict=False)
    except Exception as e:  # pylint: disable=broad-except
      print(f"Failed to load model state_dict: {e}")
      print("Using randomly initialized model for evaluation.")
  else:
    print("Using randomly initialized model for evaluation.")

  model.eval()

  try:
    x, label = load_sample(npz_path, sample_idx)
  except Exception as e:  # pylint: disable=broad-except
    print(f"Failed to load sample: {e}")
    print("Generating dummy sample for evaluation.")
    x = torch.randn(1, n_mfcc, 48 if n_mfcc == 10 else 101)
    label = 0

  start_time = time.perf_counter()
  with torch.no_grad():
    output = model(x)
  end_time = time.perf_counter()

  latency_ms = (end_time - start_time) * 1000.0
  pred = int(torch.argmax(output, dim=1).item())
  is_correct = pred == label

  print(f"\nEvaluating Hotword Sample Index: {sample_idx}")
  print(f"True Label:      {LABELS[label]} ({label})")
  print(f"Predicted Label: {LABELS[pred]} ({pred})")
  print(f"Result:          {'CORRECT' if is_correct else 'INCORRECT'}")
  print(f"Latency:         {latency_ms:.4f} ms")


def main():
  parser = argparse.ArgumentParser(
      description="Evaluate cleartext Hotword model on a single sample."
  )
  parser.add_argument(
      "--sample_idx",
      type=int,
      default=0,
      help="Index of the sample to evaluate",
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
    evaluate_single(args.model_path, args.npz_path, args.sample_idx)
  except Exception as e:
    print(f"Error evaluating sample {args.sample_idx}: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  main()
