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

"""Tests for MnistDataset utility."""

import os
from absl.testing import absltest
import numpy as np
from demos.mnist.utils.mnist_data import MnistDataset


def resolve_path(path: str) -> str:
  """Resolves file path against current directory or runfiles."""
  if os.path.exists(path):
    return path
  for env_var in ("TEST_SRCDIR", "RUNFILES_DIR"):
    runfiles = os.environ.get(env_var)
    if runfiles:
      for prefix in ("google3", "_main", ""):
        candidate = os.path.join(runfiles, prefix, path)
        if os.path.exists(candidate):
          return candidate
  return path


class MnistDataTest(absltest.TestCase):

  def setUp(self):
    super().setUp()
    self.npz_path = resolve_path(
        "demos/mnist/data/mnist.npz"
    )

  def test_load_dataset(self):
    dataset = MnistDataset(self.npz_path)
    # MNIST test set has 10,000 samples
    self.assertEqual(len(dataset), 10000)

    # Check first sample shapes and types
    image, label = dataset[0]
    self.assertEqual(image.shape, (28, 28))
    self.assertIsInstance(label, (int, np.integer))

  def test_reshape_to_2d(self):
    dataset = MnistDataset(self.npz_path, reshape_to_2d=True)
    image, _ = dataset[0]
    self.assertEqual(image.shape, (1, 28, 28))

  def test_max_samples(self):
    dataset = MnistDataset(self.npz_path, max_samples=100)
    self.assertEqual(len(dataset), 100)


if __name__ == "__main__":
  absltest.main()
