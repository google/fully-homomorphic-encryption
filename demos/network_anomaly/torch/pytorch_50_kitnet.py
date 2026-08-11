"""PyTorch implementation of 50-feature KitNET using a Unified Block-Diagonal Ensemble Linear Layer."""

import pathlib
import struct
from typing import Tuple

import numpy as np
import torch
from torch import nn

Path = pathlib.Path


class PyTorch50KitNET(nn.Module):
  """50-Feature PyTorch KitNET Anomaly Detector using Unified Block-Diagonal Linear Layers.

  This architecture replaces separate per-group sub-autoencoders and
  tensor.concat
  with single unified linear transformations equipped with block-diagonal weight
  masks.
  This allows HEIR and MLIR to compile the 50-feature model without triggering
  tensor.concat / pushSliceLayoutThroughInsertSlice layout hoisting errors.
  """

  def __init__(
      self,
      num_features: int = 50,
      num_ensembles: int = 5,
      features_per_ae: int = 10,
      hidden_per_ae: int = 8,
      output_hidden: int = 5,
  ):
    super().__init__()
    self.num_features = num_features
    self.num_ensembles = num_ensembles
    self.features_per_ae = features_per_ae
    self.hidden_per_ae = hidden_per_ae
    self.tot_ensemble_hidden = num_ensembles * hidden_per_ae
    self.output_hidden = output_hidden

    # 1. Unified Ensemble Layer (50 -> 40 -> 50, Sigmoid activation)
    self.ensemble_encoder = nn.Linear(
        self.num_features, self.tot_ensemble_hidden, bias=True
    )
    self.ensemble_decoder = nn.Linear(
        self.tot_ensemble_hidden, self.num_features, bias=True
    )

    # 2. Output Anomaly Detector Layer (50 -> 5 -> 50, Tanh activation)
    self.output_encoder = nn.Linear(
        self.num_features, self.output_hidden, bias=True
    )
    self.output_decoder = nn.Linear(
        self.output_hidden, self.num_features, bias=True
    )

    # Initialize weights with block-diagonal masks applied directly
    self.apply_masks()

  def _create_block_mask(
      self, out_dim: int, in_dim: int, out_block: int, in_block: int
  ) -> torch.Tensor:
    """Creates a block-diagonal binary mask tensor."""
    mask = torch.zeros(out_dim, in_dim, dtype=torch.float32)
    for i in range(self.num_ensembles):
      r_start = i * out_block
      r_end = min(r_start + out_block, out_dim)
      c_start = i * in_block
      c_end = min(c_start + in_block, in_dim)
      mask[r_start:r_end, c_start:c_end] = 1.0
    return mask

  def apply_masks(self) -> None:
    """Applies block-diagonal sparsity masks directly to ensemble weight matrices.

    Zeroes out off-diagonal parameters without creating registered module
    buffers.
    """
    with torch.no_grad():
      enc_mask = self._create_block_mask(
          self.tot_ensemble_hidden,
          self.num_features,
          self.hidden_per_ae,
          self.features_per_ae,
      )
      dec_mask = self._create_block_mask(
          self.num_features,
          self.tot_ensemble_hidden,
          self.features_per_ae,
          self.hidden_per_ae,
      )
      self.ensemble_encoder.weight.mul_(enc_mask)
      self.ensemble_decoder.weight.mul_(dec_mask)

  def forward(self, x: torch.Tensor) -> Tuple[torch.Tensor, torch.Tensor]:
    """Forward execution of unified 50-feature KitNET model.

    Args:
        x: Input tensor of shape (batch_size, 50) or (50,)

    Returns:
        sse_scores: Per-packet Sum of Squared Errors (batch_size,). Divide by
          num_features (50) outside the model to compute MSE.
        ad_reconstructed: Final reconstructed residual vector of shape
        (batch_size, 50).
    """
    if x.dim() == 1:
      x = x.unsqueeze(0)

    # 1. Tier 1: Unified Ensemble AutoEncoder (Sigmoid)
    #    (batch_size, 50) -> (batch_size, 40) -> (batch_size, 50)
    h_ens = torch.sigmoid(self.ensemble_encoder(x))
    x_hat = torch.sigmoid(self.ensemble_decoder(h_ens))
    r_ens = (
        x - x_hat
    )  # Direct 50-dim residual without tensor.concat / insert_slice!

    # 2. Tier 2: Output Anomaly Detector AutoEncoder (Tanh)
    #    (batch_size, 50) -> (batch_size, 5) -> (batch_size, 50)
    h_ad = torch.tanh(self.output_encoder(r_ens))
    r_hat = torch.tanh(self.output_decoder(h_ad))
    r_final = r_ens - r_hat

    # 3. Sum of Squared Errors per packet
    sse_scores = torch.sum(r_final**2, dim=1)
    return sse_scores, r_hat

  def save_weights(self, path: str | Path) -> None:
    """Save PyTorch state dict and model architecture metadata."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    checkpoint = {
        "num_features": self.num_features,
        "num_ensembles": self.num_ensembles,
        "features_per_ae": self.features_per_ae,
        "hidden_per_ae": self.hidden_per_ae,
        "output_hidden": self.output_hidden,
        "state_dict": self.state_dict(),
    }
    torch.save(checkpoint, str(path))
    print(f"✓ Saved 50-feature PyTorch KitNET model to {path}")

  @classmethod
  def load_weights(cls, path: str | Path) -> "PyTorch50KitNET":
    """Load PyTorch 50-feature KitNET model from checkpoint."""
    checkpoint = torch.load(str(path))
    model = cls(
        num_features=checkpoint.get("num_features", 50),
        num_ensembles=checkpoint.get("num_ensembles", 5),
        features_per_ae=checkpoint.get("features_per_ae", 10),
        hidden_per_ae=checkpoint.get("hidden_per_ae", 8),
        output_hidden=checkpoint.get("output_hidden", 5),
    )
    state_dict = checkpoint.get("state_dict", checkpoint)
    model.load_state_dict(state_dict, strict=False)
    model.apply_masks()
    return model

  @classmethod
  def load_from_binary_model(cls, model_path: str | Path) -> "PyTorch50KitNET":
    """Load weights directly from Niobium's legacy binary .bin model file."""
    model_path = Path(model_path)
    hdrfmt = "@" + "7h"
    hdrlen = 7 * 2

    with open(model_path, "rb") as fp:
      hdr_bytes = fp.read(hdrlen)
      k_ensembles, n_feat, dae, hae, dad, had, ord_apx = struct.unpack(
          hdrfmt, hdr_bytes
      )

      apxlen = (ord_apx + 1) * 8
      fp.read(apxlen)  # Skip sigmoid cheby coeffs
      fp.read(apxlen)  # Skip tanh cheby coeffs

      # Skip feature map stored in binary file
      maplen = dae * 2
      for _ in range(k_ensembles):
        fp.read(maplen)

      model = cls(
          num_features=n_feat,
          num_ensembles=k_ensembles,
          features_per_ae=dae,
          hidden_per_ae=hae,
          output_hidden=had,
      )

      aehidlen = hae * 8
      aehidfmt = "@" + str(hae) + "d"
      aevislen = dae * 8
      aevisfmt = "@" + str(dae) + "d"

      # Initialize unified weight and bias matrices
      w_enc_full = np.zeros((k_ensembles * hae, n_feat), dtype=np.float32)
      b_enc_full = np.zeros((k_ensembles * hae,), dtype=np.float32)
      w_dec_full = np.zeros((n_feat, k_ensembles * hae), dtype=np.float32)
      b_dec_full = np.zeros((n_feat,), dtype=np.float32)

      for a in range(k_ensembles):
        w_hat = np.zeros((dae, hae), dtype=np.float64)
        for ii in range(dae):
          bits = fp.read(aehidlen)
          w_hat[ii] = struct.unpack(aehidfmt, bits)

        bits = fp.read(aehidlen)
        hbias = np.array(struct.unpack(aehidfmt, bits), dtype=np.float64)

        bits = fp.read(aevislen)
        rbias = np.array(struct.unpack(aevisfmt, bits), dtype=np.float64)

        r_start_hid = a * hae
        r_end_hid = (a + 1) * hae
        c_start_vis = a * dae
        c_end_vis = (a + 1) * dae

        # Linear weights in PyTorch are (out_features, in_features)
        w_enc_full[r_start_hid:r_end_hid, c_start_vis:c_end_vis] = (
            w_hat.T.astype(np.float32)
        )
        b_enc_full[r_start_hid:r_end_hid] = hbias.astype(np.float32)

        w_dec_full[c_start_vis:c_end_vis, r_start_hid:r_end_hid] = w_hat.astype(
            np.float32
        )
        b_dec_full[c_start_vis:c_end_vis] = rbias.astype(np.float32)

      with torch.no_grad():
        model.ensemble_encoder.weight.copy_(torch.from_numpy(w_enc_full))
        model.ensemble_encoder.bias.copy_(torch.from_numpy(b_enc_full))
        model.ensemble_decoder.weight.copy_(torch.from_numpy(w_dec_full))
        model.ensemble_decoder.bias.copy_(torch.from_numpy(b_dec_full))

      # Read Output AutoEncoder
      adhidlen = had * 8
      adhidfmt = "@" + str(had) + "d"
      advislen = dad * 8
      advisfmt = "@" + str(dad) + "d"

      w_hat_ad = np.zeros((dad, had), dtype=np.float64)
      for ii in range(dad):
        bits = fp.read(adhidlen)
        w_hat_ad[ii] = struct.unpack(adhidfmt, bits)

      bits = fp.read(adhidlen)
      hbias_ad = np.array(struct.unpack(adhidfmt, bits), dtype=np.float64)

      bits = fp.read(advislen)
      rbias_ad = np.array(struct.unpack(advisfmt, bits), dtype=np.float64)

      with torch.no_grad():
        model.output_encoder.weight.copy_(
            torch.from_numpy(w_hat_ad.T.astype(np.float32))
        )
        model.output_encoder.bias.copy_(
            torch.from_numpy(hbias_ad.astype(np.float32))
        )
        model.output_decoder.weight.copy_(
            torch.from_numpy(w_hat_ad.astype(np.float32))
        )
        model.output_decoder.bias.copy_(
            torch.from_numpy(rbias_ad.astype(np.float32))
        )

    model.apply_masks()
    return model
