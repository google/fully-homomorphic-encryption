"""Canonical MLP network definition for MNIST demo."""

import torch.nn as nn


class CanonicalMLP(nn.Module):
  """A 3-layer MLP model for MNIST classification."""

  def __init__(self):
    super().__init__()
    # Flatten input is handled in forward, layers defined here
    self.fc1 = nn.Linear(784, 512)  # Input to Hidden 1
    self.fc2 = nn.Linear(512, 512)  # Hidden 1 to Hidden 2
    self.fc3 = nn.Linear(512, 10)  # Hidden 2 to Output

    self.dropout = nn.Dropout(0.2)  # Standard regularization
    self.relu = nn.ReLU()  # Standard activation

  def forward(self, x):
    # Flatten image: (Batch_Size, 1, 28, 28) -> (Batch_Size, 784)
    x = x.view(-1, 28 * 28)

    # Layer 1
    x = self.relu(self.fc1(x))
    x = self.dropout(x)

    # Layer 2
    x = self.relu(self.fc2(x))
    x = self.dropout(x)

    # Output Layer (Raw logits, CrossEntropyLoss handles Softmax)
    x = self.fc3(x)
    return x
