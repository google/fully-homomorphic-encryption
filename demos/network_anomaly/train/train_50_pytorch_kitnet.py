"""Training script for 50-feature PyTorch KitNET with Unified Block-Diagonal Ensemble Layer."""

import argparse
import pathlib

import numpy as np
import torch

from demos.common.python.path_utils import resolve_path
from demos.network_anomaly.torch.pytorch_50_kitnet import PyTorch50KitNET

Path = pathlib.Path
nn = torch.nn
optim = torch.optim


def load_binary_dataset(
    data_file: str | Path, num_packets: int, num_features: int = 50
) -> np.ndarray:
  """Loads 50-feature dataset from binary double (float64) packet file."""
  data_path = Path(resolve_path(str(data_file)))
  if not data_path.exists():
    raise FileNotFoundError(f"Dataset file not found: {data_path}")

  print(f"Loading binary dataset from: {data_path}")
  with open(data_path, "rb") as f:
    raw_bytes = f.read(num_packets * num_features * 8)
    num_read = len(raw_bytes) // (num_features * 8)
    data = (
        np.frombuffer(raw_bytes, dtype=np.float64)
        .reshape((num_read, num_features))
        .copy()
    )
    print(
        f"Successfully loaded {data.shape[0]} packets with {data.shape[1]}"
        " features each."
    )
    return data


def train_50_pytorch_model(
    data_file: str | Path,
    num_packets: int = 32768,
    epochs: int = 10,
    lr: float = 0.01,
    batch_size: int = 64,
    output_model_path: str = "demos/network_anomaly/data/torch_50_kitnet_model.pt",
) -> PyTorch50KitNET:
  """Trains unified block-diagonal 50-feature PyTorch KitNET model."""
  print("=" * 80)
  print("  Training Unified Block-Diagonal 50-Feature PyTorch KitNET Model")
  print("=" * 80)
  print(f"Dataset File:   {data_file}")
  print(f"Target Packets: {num_packets}")
  print(f"Epochs:         {epochs}")
  print(f"Learning Rate:  {lr}")
  print(f"Batch Size:     {batch_size}")
  print(f"Output Path:    {output_model_path}\n")

  # 1. Load 50-Feature Training Data
  data_np = load_binary_dataset(data_file, num_packets, num_features=50)
  data_tensor = torch.from_numpy(data_np).float()
  dataset_size = data_tensor.shape[0]

  # 2. Instantiate Unified 50-Feature Model
  model = PyTorch50KitNET(
      num_features=50,
      num_ensembles=5,
      features_per_ae=10,
      hidden_per_ae=8,
      output_hidden=5,
  )
  print(
      "✓ Initialized PyTorch50KitNET (50 -> 40 -> 50 Ensemble, 50 -> 5 -> 50"
      " Output AE)."
  )

  # 3. Phase 1: Train Ensemble Layer AutoEncoders (Sigmoid)
  print("\n--- Phase 1: Training Unified Block-Diagonal Ensemble Layer ---")
  ensemble_params = list(model.ensemble_encoder.parameters()) + list(
      model.ensemble_decoder.parameters()
  )
  ensemble_optimizer = optim.Adam(ensemble_params, lr=lr)
  criterion = nn.MSELoss()

  model.train()
  for epoch in range(1, epochs + 1):
    epoch_loss = 0.0
    num_batches = 0

    perm = torch.randperm(dataset_size)
    for b in range(0, dataset_size, batch_size):
      batch_idx = perm[b : b + batch_size]
      batch_x = data_tensor[batch_idx]

      ensemble_optimizer.zero_grad()

      h = torch.sigmoid(model.ensemble_encoder(batch_x))
      x_hat = torch.sigmoid(model.ensemble_decoder(h))

      loss = criterion(x_hat, batch_x)
      loss.backward()
      ensemble_optimizer.step()

      # Enforce block-diagonal sparsity after optimizer step
      model.apply_masks()

      epoch_loss += loss.item()
      num_batches += 1

    avg_loss = epoch_loss / num_batches
    if epoch % 2 == 0 or epoch == epochs:
      print(
          f"  Ensemble Layer | Epoch [{epoch:2d}/{epochs:2d}] | MSE Loss:"
          f" {avg_loss:.6e}"
      )

  # 4. Phase 2: Train Output Anomaly Detector Layer (Tanh)
  print("\n--- Phase 2: Training Output Anomaly Detector Layer ---")
  model.eval()
  with torch.no_grad():
    h_ens = torch.sigmoid(model.ensemble_encoder(data_tensor))
    x_hat = torch.sigmoid(model.ensemble_decoder(h_ens))
    ensemble_residuals = data_tensor - x_hat

  output_params = list(model.output_encoder.parameters()) + list(
      model.output_decoder.parameters()
  )
  output_optimizer = optim.Adam(output_params, lr=lr)

  model.train()
  for epoch in range(1, epochs + 1):
    epoch_loss = 0.0
    num_batches = 0

    perm = torch.randperm(dataset_size)
    for b in range(0, dataset_size, batch_size):
      batch_idx = perm[b : b + batch_size]
      batch_r = ensemble_residuals[batch_idx]

      output_optimizer.zero_grad()

      h_ad = torch.tanh(model.output_encoder(batch_r))
      r_hat = torch.tanh(model.output_decoder(h_ad))

      loss = criterion(r_hat, batch_r)
      loss.backward()
      output_optimizer.step()

      epoch_loss += loss.item()
      num_batches += 1

    avg_loss = epoch_loss / num_batches
    if epoch % 2 == 0 or epoch == epochs:
      print(
          f"  Output AE Layer | Epoch [{epoch:2d}/{epochs:2d}] | MSE Loss:"
          f" {avg_loss:.6e}"
      )

  # 5. Evaluate Final Training Metrics
  model.eval()
  with torch.no_grad():
    sse_scores, _ = model(data_tensor)
    mse_scores = (sse_scores / 50.0).numpy()

  print("\n" + "=" * 80)
  print("  50-Feature Training Results Summary")
  print("=" * 80)
  print(f"Average Reconstruction MSE: {np.mean(mse_scores):.6e}")
  print(f"Min Reconstruction MSE:     {np.min(mse_scores):.6e}")
  print(f"Max Reconstruction MSE:     {np.max(mse_scores):.6e}")
  print(f"Std Dev Reconstruction MSE: {np.std(mse_scores):.6e}\n")

  # 6. Save Model Checkpoint
  resolved_out = Path(resolve_path(output_model_path))
  resolved_out.parent.mkdir(parents=True, exist_ok=True)
  model.save_weights(str(resolved_out))
  return model


def main():
  parser = argparse.ArgumentParser(
      description=(
          "Train 50-feature PyTorch KitNET with Unified Block-Diagonal Layers."
      )
  )
  parser.add_argument(
      "--data-file",
      type=str,
      default="demos/network_anomaly/data/Mirai_full_50_features_32K.bin",
      help="Path to 50-feature binary dataset file",
  )
  parser.add_argument(
      "--packets",
      type=int,
      default=32768,
      help="Number of packets to train on (default: 32768)",
  )
  parser.add_argument(
      "--epochs",
      type=int,
      default=10,
      help="Training epochs per layer (default: 10)",
  )
  parser.add_argument(
      "--lr",
      type=float,
      default=0.01,
      help="Learning rate for Adam optimizer (default: 0.01)",
  )
  parser.add_argument(
      "--batch-size",
      type=int,
      default=64,
      help="Batch size for training (default: 64)",
  )
  parser.add_argument(
      "--output",
      type=str,
      default="demos/network_anomaly/data/torch_50_kitnet_model.pt",
      help="Output PyTorch checkpoint path",
  )
  args = parser.parse_args()

  train_50_pytorch_model(
      data_file=args.data_file,
      num_packets=args.packets,
      epochs=args.epochs,
      lr=args.lr,
      batch_size=args.batch_size,
      output_model_path=args.output,
  )


if __name__ == "__main__":
  main()
