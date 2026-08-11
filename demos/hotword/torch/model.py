"""TC-ResNet: Temporal Convolutional ResNet for keyword spotting / wake-word detection.

Reference:
    S. Choi et al., "Temporal Convolution for Real-time Keyword Spotting on
    Mobile Devices," INTERSPEECH 2019 (Qualcomm AI Research).

Core idea
---------
The MFCC feature map of shape (n_mfcc, time) is fed to 1-D convolutions that
slide ONLY along the time axis. The MFCC coefficients are placed on the
*channel* dimension, so the network input is (batch, n_mfcc, time). Putting
frequency on the channel axis lets the temporal receptive field grow quickly
while keeping the model tiny -- ideal for always-on, on-device detection.
"""

from typing import List, Sequence

import torch
import torch.nn as nn


class LayerMarker(nn.Module):
  """Identity module to mark layers for calibration."""

  def forward(self, x: torch.Tensor) -> torch.Tensor:
    return x


class ResBlock(nn.Module):
  """TC-ResNet residual block: two temporal convs with a projection shortcut."""

  def __init__(
      self,
      in_channels: int,
      out_channels: int,
      kernel_size: int = 9,
      stride: int = 1,
  ):
    super().__init__()
    pad = kernel_size // 2

    self.conv1 = nn.Conv1d(
        in_channels,
        out_channels,
        kernel_size,
        stride=stride,
        padding=pad,
        bias=False,
    )
    self.bn1 = nn.BatchNorm1d(out_channels)
    self.conv2 = nn.Conv1d(
        out_channels,
        out_channels,
        kernel_size,
        stride=1,
        padding=pad,
        bias=False,
    )
    self.bn2 = nn.BatchNorm1d(out_channels)

    # Layer Markers
    self.marker_relu1 = LayerMarker()
    self.marker_relu2 = LayerMarker()

    self.relu = nn.ReLU(inplace=True)

    # Projection shortcut when the shape changes (stride > 1 or channel change).
    if stride != 1 or in_channels != out_channels:
      self.shortcut = nn.Sequential(
          nn.Conv1d(
              in_channels,
              out_channels,
              kernel_size=1,
              stride=stride,
              bias=False,
          ),
          nn.BatchNorm1d(out_channels),
      )
    else:
      self.shortcut = nn.Identity()

  def forward(self, x: torch.Tensor) -> torch.Tensor:
    identity = self.shortcut(x)
    out = self.bn1(self.conv1(x))
    out = self.marker_relu1(out)
    out = self.relu(out)

    out = self.bn2(self.conv2(out))
    out = out + identity
    out = self.marker_relu2(out)
    out = self.relu(out)
    return out


class TCResNet(nn.Module):
  """Configurable TC-ResNet.

  Args:
      n_mfcc:         number of MFCC coefficients (== number of input channels).
      num_classes:    number of output classes (keywords + silence/unknown).
      channels:       output channels for each residual block.
      strides:        temporal stride for each residual block (same length as
        channels).
      first_channels: output channels of the initial conv layer.
      first_kernel:   kernel size of the initial conv layer.
      block_kernel:   temporal kernel size inside the residual blocks.
      width_mult:     scales every channel count (paper reports 1.0 and 1.5).
  """

  def __init__(
      self,
      n_mfcc: int = 40,
      num_classes: int = 12,
      channels: Sequence[int] = (24, 32, 48),
      strides: Sequence[int] = (2, 2, 2),
      first_channels: int = 16,
      first_kernel: int = 3,
      block_kernel: int = 9,
      width_mult: float = 1.0,
  ):
    super().__init__()
    if len(channels) != len(strides):
      raise ValueError("`channels` and `strides` must have the same length")

    def scale(c: int) -> int:
      return max(1, int(round(c * width_mult)))

    first_channels = scale(first_channels)
    channels = [scale(c) for c in channels]

    # Initial temporal conv (no downsampling).
    self.stem_conv = nn.Conv1d(
        n_mfcc,
        first_channels,
        first_kernel,
        stride=1,
        padding=first_kernel // 2,
        bias=False,
    )
    self.stem_bn = nn.BatchNorm1d(first_channels)
    self.marker_stem = LayerMarker()
    self.relu = nn.ReLU(inplace=True)

    # Residual stages.
    blocks: List[nn.Module] = []
    in_channels = first_channels
    for out_channels, stride in zip(channels, strides):
      blocks.append(
          ResBlock(
              in_channels, out_channels, kernel_size=block_kernel, stride=stride
          )
      )
      in_channels = out_channels
    self.blocks = nn.Sequential(*blocks)

    # Classifier: global average pool over time -> linear.
    self.pool = nn.AdaptiveAvgPool1d(1)
    self.flatten = nn.Flatten()
    self.fc = nn.Linear(in_channels, num_classes)

    self._init_weights()

  def _init_weights(self) -> None:
    for m in self.modules():
      if isinstance(m, nn.Conv1d):
        nn.init.kaiming_normal_(m.weight, mode="fan_out", nonlinearity="relu")
      elif isinstance(m, nn.BatchNorm1d):
        nn.init.ones_(m.weight)
        nn.init.zeros_(m.bias)
      elif isinstance(m, nn.Linear):
        nn.init.normal_(m.weight, 0, 0.01)
        nn.init.zeros_(m.bias)

  def forward(self, x: torch.Tensor) -> torch.Tensor:
    """x: (batch, n_mfcc, time) -> logits: (batch, num_classes).

    A 4-D input of shape (batch, 1, n_mfcc, time) is also accepted and the
    singleton channel is squeezed away.
    """
    if x.dim() == 4:  # (B, 1, n_mfcc, T) -> (B, n_mfcc, T)
      x = x.squeeze(1)
    x = self.relu(self.marker_stem(self.stem_bn(self.stem_conv(x))))
    x = self.blocks(x)
    x = self.pool(x)
    x = self.flatten(x)
    return self.fc(x)


# --------------------------------------------------------------------------- #
# Factory functions for the paper's variants
# --------------------------------------------------------------------------- #


def tc_resnet8(
    n_mfcc: int = 40, num_classes: int = 12, width_mult: float = 1.0
) -> TCResNet:
  """TC-ResNet8: 3 residual blocks."""
  return TCResNet(
      n_mfcc=n_mfcc,
      num_classes=num_classes,
      channels=(24, 32, 48),
      strides=(2, 2, 2),
      width_mult=width_mult,
  )


def tc_resnet8_small(
    n_mfcc: int = 10, num_classes: int = 12, width_mult: float = 1.0
) -> TCResNet:
  """TC-ResNet8 on the DS-CNN front end: 10 MFCCs x 48 frames."""
  return TCResNet(
      n_mfcc=n_mfcc,
      num_classes=num_classes,
      channels=(24, 32, 48),
      strides=(2, 2, 2),
      width_mult=width_mult,
  )


def tc_resnet8_large(
    n_mfcc: int = 40, num_classes: int = 12, width_mult: float = 1.0
) -> TCResNet:
  """TC-ResNet8 on the paper's front end: 40 MFCCs x 101 frames."""
  return TCResNet(
      n_mfcc=n_mfcc,
      num_classes=num_classes,
      channels=(24, 32, 48),
      strides=(2, 2, 2),
      width_mult=width_mult,
  )


def tc_resnet14(
    n_mfcc: int = 40, num_classes: int = 12, width_mult: float = 1.0
) -> TCResNet:
  """TC-ResNet14: 6 residual blocks; higher accuracy, still small."""
  return TCResNet(
      n_mfcc=n_mfcc,
      num_classes=num_classes,
      channels=(24, 24, 32, 32, 48, 48),
      strides=(2, 1, 2, 1, 2, 1),
      width_mult=width_mult,
  )
