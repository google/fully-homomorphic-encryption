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

"""Calibrate intermediate values of Hotword model."""

import argparse
import csv
import os
import sys

import torch
from torch.utils.data import DataLoader

from demos.common.python import path_utils
from demos.hotword.torch import model as tc_resnet
from demos.hotword.torch.model import LayerMarker
from demos.hotword.utils import hotword_data
from demos.hotword.utils.hotword_data import HotwordDataset

LABELS = hotword_data.LABELS
resolve_path = path_utils.resolve_path


def main():
  parser = argparse.ArgumentParser(
      description="Calibrate intermediate values of Hotword model."
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
  parser.add_argument(
      "--batch_size",
      type=int,
      default=64,
      help="Batch size for evaluation during calibration",
  )
  parser.add_argument(
      "--out_csv",
      type=str,
      default=None,
      help="Path to save calibration results as CSV",
  )
  args = parser.parse_args()

  resolved_model_path = resolve_path(args.model_path)
  if not os.path.exists(resolved_model_path):
    raise FileNotFoundError(f"Model file not found at {args.model_path}")

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

  # Fallback to random weights if placeholder is empty
  if state is not None:
    try:
      model.load_state_dict(state, strict=False)
    except Exception as e:  # pylint: disable=broad-except
      print(f"Warning: Failed to load model: {e}")
      print("Using randomly initialized model for calibration.")
  else:
    print("Using randomly initialized model for calibration.")

  model.eval()

  resolved_npz_path = resolve_path(args.npz_path)
  print(f"Loading test data from: {resolved_npz_path}")
  try:
    dataset = HotwordDataset(npz_path=resolved_npz_path)
  except Exception as e:  # pylint: disable=broad-except
    print(f"Warning: Failed to load dataset: {e}")
    print("Generating dummy dataset for calibration.")

    # Create a dummy dataset with same structure
    class DummyDataset(torch.utils.data.Dataset):

      def __init__(self):
        self.x = torch.randn(100, n_mfcc, 48 if n_mfcc == 10 else 101)
        self.y = torch.zeros(100, dtype=torch.long)

      def __len__(self):
        return len(self.x)

      def __getitem__(self, idx):
        return self.x[idx], self.y[idx]

    dataset = DummyDataset()

  dataloader = DataLoader(dataset, batch_size=args.batch_size, shuffle=False)

  # Dictionary to store collected values for each marker
  collected_values = {}

  def get_hook(name):
    def hook(module, input_val, output_val):  # pylint: disable=unused-argument
      # input_val is a tuple. The first element is the input tensor.
      val = input_val[0].clone().detach().cpu().flatten()
      if name not in collected_values:
        collected_values[name] = []
      collected_values[name].append(val)

    return hook

  # Register hooks on all LayerMarker modules
  markers_found = 0
  for name, module in model.named_modules():
    if isinstance(module, LayerMarker):
      module.register_forward_hook(get_hook(name))
      markers_found += 1

  print(f"Registered hooks on {markers_found} LayerMarker modules.")

  # Run inference on the dataset
  print("Running inference...")
  with torch.no_grad():
    for x, _ in dataloader:
      _ = model(x)

  # Compute and print statistics
  print("\n--- Calibration Results ---")
  print(
      f"{'Layer Name':<40} | {'Min':<10} | {'Max':<10} | {'5th %':<10} |"
      f" {'95th %':<10}"
  )
  print("-" * 90)

  csv_rows = []

  for name, vals in collected_values.items():
    all_vals = torch.cat(vals)

    if all_vals.numel() == 0:
      print(f"{name:<40} | No values collected")
      continue

    min_val = all_vals.min().item()
    max_val = all_vals.max().item()
    p5 = torch.quantile(all_vals, 0.05).item()
    p95 = torch.quantile(all_vals, 0.95).item()

    print(
        f"{name:<40} | {min_val:<10.4f} | {max_val:<10.4f} | {p5:<10.4f} |"
        f" {p95:<10.4f}"
    )
    if args.out_csv:
      csv_rows.append([name, min_val, max_val, p5, p95])

  if args.out_csv:
    out_path = args.out_csv
    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    print(f"\nWriting calibration results to CSV: {out_path}")
    with open(out_path, "w", newline="") as f:
      writer = csv.writer(f)
      writer.writerow([
          "layer_name",
          "min",
          "max",
          "5th_percentile",
          "95th_percentile",
      ])
      writer.writerows(csv_rows)


if __name__ == "__main__":
  main()
