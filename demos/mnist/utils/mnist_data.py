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

"""MNIST dataset loader utility for NPZ format."""

import numpy as np


class MnistDataset:
  """Loads MNIST dataset from a .npz file."""

  def __init__(
      self,
      npz_path: str,
      reshape_to_2d: bool = False,
      max_samples: int | None = None,
  ):
    with np.load(npz_path) as data:
      # tensorflow_io mnist.npz uses x_test and y_test keys
      self.images = data["x_test"].astype(np.float32)
      self.labels = data["y_test"].astype(np.int64)

    # Normalize images: (val / 255.0 - 0.1307) / 0.3081
    self.images = (self.images / 255.0 - 0.1307) / 0.3081

    if max_samples is not None:
      self.images = self.images[:max_samples]
      self.labels = self.labels[:max_samples]

    if reshape_to_2d:
      self.images = self.images.reshape(-1, 1, 28, 28)

  def __len__(self):
    return len(self.images)

  def __getitem__(self, idx):
    return self.images[idx], self.labels[idx]
