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

"""Convert a PyTorch TC-ResNet8 model to MLIR."""

import argparse
import os
import sys

import torch

from demos.common.python import export_mlir_utils
from demos.common.python import path_utils
from demos.hotword.torch import model as tc_resnet
from demos.hotword.utils import hotword_data

LABELS = hotword_data.LABELS
resolve_path = path_utils.resolve_path


def main():
  parser = argparse.ArgumentParser(
      description="Convert a PyTorch hotword model to MLIR."
  )
  parser.add_argument(
      "--input_model",
      default="demos/hotword/data/tc_resnet8.pth",
      help="Path to the input PyTorch model file",
  )
  parser.add_argument(
      "--output_file",
      default=(
          "demos/hotword/data/hotword.mlir"
      ),
      help="Path where the resulting .mlir file will be saved",
  )

  args = parser.parse_args()

  input_path = resolve_path(args.input_model)

  model = tc_resnet.tc_resnet8(n_mfcc=40, num_classes=len(LABELS))

  loaded = False
  if os.path.exists(input_path):
    try:
      model.load_state_dict(
          torch.load(input_path, map_location="cpu"), strict=False
      )
      print(f"Successfully loaded model from {input_path}")
      loaded = True
    except Exception as e:  # pylint: disable=broad-except
      print(f"Warning: Failed to load model from {input_path}: {e}")
  else:
    print(f"Warning: Input model not found at {input_path}")

  if not loaded:
    print("Using randomly initialized model for export.")

  model.eval()

  # Input shape: (1, 40, 101) (batch, n_mfcc, time)
  example_input = torch.randn(1, 40, 101)

  # Use common utility for export and post-processing
  export_mlir_utils.export_and_postprocess(
      model,
      example_input,
      args.output_file,
      entrypoint_name="hotword",
      use_fx=False,
  )


if __name__ == "__main__":
  main()
