"""PyTorch implementation of KitNET Ensemble Anomaly Detector."""

# NOTE: Uses native PyTorch activations (torch.sigmoid and torch.tanh)
# for HEIR compiler evaluation.

import pathlib
import struct

import numpy as np
import torch
from torch import nn

Path = pathlib.Path


class PyTorchAutoEncoder(nn.Module):
  """Single Denoising Autoencoder layer using PyTorch native activations."""

  def __init__(self, n_visible: int, n_hidden: int, nonlin: str = 'sigmoid'):
    super().__init__()
    self.n_visible = n_visible
    self.n_hidden = n_hidden
    self.nonlin = nonlin

    self.encoder = nn.Linear(n_visible, n_hidden, bias=True)
    self.decoder = nn.Linear(n_hidden, n_visible, bias=True)

    if nonlin == 'sigmoid':
      self.activation = nn.Sigmoid()
    elif nonlin == 'tanh':
      self.activation = nn.Tanh()
    else:
      raise ValueError(f'Unsupported non-linearity: {nonlin}')

  def forward(self, x: torch.Tensor) -> torch.Tensor:
    hidden = self.activation(self.encoder(x))
    reconstructed = self.activation(self.decoder(hidden))
    return reconstructed

  def load_from_numpy(
      self, weight: np.ndarray, hbias: np.ndarray, rbias: np.ndarray
  ):
    """Load weight matrix (shape: n_visible x n_hidden) and biases."""
    with torch.no_grad():
      self.encoder.weight.copy_(torch.from_numpy(weight.T).float())
      self.encoder.bias.copy_(torch.from_numpy(hbias).float())
      self.decoder.weight.copy_(torch.from_numpy(weight).float())
      self.decoder.bias.copy_(torch.from_numpy(rbias).float())


class PyTorchKitNET(nn.Module):
  """PyTorch implementation of KitNET Ensemble Anomaly Detector.

  Uses native PyTorch activations without manual polynomial approximations.
  """

  def __init__(
      self, num_features: int, feature_map: list[list[int]] | None = None
  ):
    super().__init__()
    self.num_features = num_features
    self.feature_map = feature_map or []
    self.ensemble_layers = nn.ModuleList()
    self.output_layer: PyTorchAutoEncoder | None = None

    if feature_map:
      self._build_architecture()

  def _build_architecture(self):
    self.ensemble_layers = nn.ModuleList()
    for fmap in self.feature_map:
      n_vis = len(fmap)
      n_hid = int(np.ceil(n_vis * 0.75))
      self.ensemble_layers.append(
          PyTorchAutoEncoder(n_vis, n_hid, nonlin='sigmoid')
      )

    tot_ensemble_vis = sum(len(fmap) for fmap in self.feature_map)
    n_ad_hid = len(self.feature_map)
    self.output_layer = PyTorchAutoEncoder(
        tot_ensemble_vis, n_ad_hid, nonlin='tanh'
    )

    # Contiguous slice bounds for sequential ensemble feature mapping (Case A)
    self.slice_bounds = []
    offset = 0
    for fmap in self.feature_map:
      length = len(fmap)
      self.slice_bounds.append((offset, offset + length))
      offset += length

  def permute_input(self, x: torch.Tensor) -> torch.Tensor:
    """Passes input features directly in sequential order (Case A)."""
    if x.dim() == 1:
      x = x.unsqueeze(0)
    return x

  def forward(self, x: torch.Tensor) -> tuple[torch.Tensor, torch.Tensor]:
    """Forward execution of PyTorch KitNET.

    Args:
        x: Input tensor of shape (batch_size, num_features) or (num_features,)

    Returns:
        sse_scores: Per-packet sum of squared errors (batch_size,). Divide
          by num_features outside the model to compute MSE.
        ad_reconstructed: Final reconstructed residual vector
    """
    if x.dim() == 1:
      x = x.unsqueeze(0)

    residuals = []

    # 1. Ensemble Layer execution using contiguous sequential range slicing
    for idx, ae in enumerate(self.ensemble_layers):
      start, end = self.slice_bounds[idx]
      sub_x = x[:, start:end]
      sub_reconstructed = ae(sub_x)
      sub_residual = sub_x - sub_reconstructed
      residuals.append(sub_residual)

    ensemble_residual = torch.cat(residuals, dim=1)

    # 2. Output Anomaly Detector execution
    assert self.output_layer is not None
    ad_reconstructed = self.output_layer(ensemble_residual)
    ad_residual = ensemble_residual - ad_reconstructed

    # Compute sum of squared error (SSE) per packet (division by num_features
    # is performed outside the model for MLIR/FHE compatibility)
    sse_scores = torch.sum(ad_residual**2, dim=1)
    return sse_scores, ad_reconstructed

  def save_weights(self, path: str | Path):
    """Save PyTorch state dict and model metadata."""
    path = Path(path)
    checkpoint = {
        'num_features': self.num_features,
        'feature_map': self.feature_map,
        'state_dict': self.state_dict(),
    }
    torch.save(checkpoint, str(path))

  @classmethod
  def load_weights(cls, path: str | Path) -> 'PyTorchKitNET':
    """Load PyTorch KitNET model from saved checkpoint."""
    checkpoint = torch.load(str(path))
    model = cls(checkpoint['num_features'], checkpoint['feature_map'])
    state_dict = checkpoint.get('state_dict', checkpoint)
    # Filter out legacy buffer keys if present in previous checkpoint files
    clean_state_dict = {
        k: v for k, v in state_dict.items() if k != 'perm_indices_tensor'
    }
    model.load_state_dict(clean_state_dict, strict=False)
    return model

  @classmethod
  def load_from_binary_model(cls, model_path: str | Path) -> 'PyTorchKitNET':
    """Load weights directly from Niobium's legacy binary .bin model file."""
    model_path = Path(model_path)
    hdrfmt = '@' + '7h'
    hdrlen = 7 * 2

    with open(model_path, 'rb') as fp:
      hdr_bytes = fp.read(hdrlen)
      k_ensembles, n_feat, dae, hae, dad, had, ord_apx = struct.unpack(
          hdrfmt, hdr_bytes
      )

      apxlen = (ord_apx + 1) * 8
      fp.read(apxlen)  # Skip sigmoid cheby coeffs
      fp.read(apxlen)  # Skip tanh cheby coeffs

      feature_map = []
      maplen = dae * 2
      mapfmt = '@' + str(dae) + 'h'
      for _ in range(k_ensembles):
        bits = fp.read(maplen)
        mapfeats = list(struct.unpack(mapfmt, bits))
        feature_map.append(mapfeats)

      model = cls(n_feat, feature_map)

      aehidlen = hae * 8
      aehidfmt = '@' + str(hae) + 'd'
      aevislen = dae * 8
      aevisfmt = '@' + str(dae) + 'd'

      for a in range(k_ensembles):
        w_hat = np.zeros((dae, hae))
        for ii in range(dae):
          bits = fp.read(aehidlen)
          w_hat[ii] = struct.unpack(aehidfmt, bits)

        bits = fp.read(aehidlen)
        hbias = np.array(struct.unpack(aehidfmt, bits))

        bits = fp.read(aevislen)
        rbias = np.array(struct.unpack(aevisfmt, bits))

        model.ensemble_layers[a].load_from_numpy(w_hat, hbias, rbias)

      adhidlen = had * 8
      adhidfmt = '@' + str(had) + 'd'
      advislen = dad * 8
      advisfmt = '@' + str(dad) + 'd'

      w_hat_ad = np.zeros((dad, had))
      for ii in range(dad):
        bits = fp.read(adhidlen)
        w_hat_ad[ii] = struct.unpack(adhidfmt, bits)

      bits = fp.read(adhidlen)
      hbias_ad = np.array(struct.unpack(adhidfmt, bits))

      bits = fp.read(advislen)
      rbias_ad = np.array(struct.unpack(advisfmt, bits))

      assert model.output_layer is not None
      model.output_layer.load_from_numpy(w_hat_ad, hbias_ad, rbias_ad)

    return model
