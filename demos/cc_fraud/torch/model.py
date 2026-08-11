"""Multi Level Perceptron (MLP) architectures for fraud detection model."""

from typing import List
import torch
import torch.nn as nn


class MLPSigmoid(nn.Module):
  """MLP with Sigmoid activations."""

  def __init__(self, input_dim: int, hidden_dims: List[int], num_classes: int):
    super().__init__()
    layers = []
    prev_dim = input_dim
    for h_dim in hidden_dims:
      layers.extend([
          nn.Linear(prev_dim, h_dim),
          nn.Sigmoid(),
      ])
      prev_dim = h_dim
    layers.append(nn.Linear(prev_dim, num_classes))
    self.net = nn.Sequential(*layers)

  def forward(self, x: torch.Tensor) -> torch.Tensor:
    return self.net(x)

  def forward_with_preacts(self, x: torch.Tensor):
    """Forward pass that also returns pre-activation values (Linear outputs, before sigmoid)."""
    preacts = []
    h = x
    for layer in self.net:
      h = layer(h)
      # Capture the output of Linear (which is the input to Sigmoid) except the last output layer
      if isinstance(layer, nn.Linear) and layer is not self.net[-1]:
        preacts.append(h)
    return h, preacts
