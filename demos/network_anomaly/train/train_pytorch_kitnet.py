"""Training script for PyTorch KitNET anomaly detection model."""

import argparse
import pathlib

import numpy as np
import torch

from demos.common.python.path_utils import resolve_path
from demos.network_anomaly.torch.pytorch_kitnet import PyTorchKitNET

Path = pathlib.Path
nn = torch.nn
optim = torch.optim


def load_binary_dataset(
    data_file: str | Path, num_packets: int, num_features: int
) -> np.ndarray:
  """Load dataset from binary double (float64) packet file."""
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
        " features each.\n"
    )
    return data


def train_pytorch_model(
    dataset: str,
    data_file: str,
    num_features: int,
    num_packets: int,
    epochs: int,
    lr: float,
    batch_size: int,
    output_model_path: str,
):
  """Train PyTorch KitNET Model."""
  print(f"TRAINING PYTORCH KITNET MODEL - {dataset.upper()} DATASET")

  # 1. Load Data
  data_np = load_binary_dataset(data_file, num_packets, num_features)
  data_tensor = torch.from_numpy(data_np).float()

  # 2. Define Feature Mapping
  if num_features == 2:
    feature_map = [[0, 1]]
  elif num_features == 5:
    feature_map = [[0, 1, 2, 3, 4]]
  else:
    feature_map = [
        list(range(i, min(i + 5, num_features)))
        for i in range(0, num_features, 5)
    ]

  print(f"Feature Map: {feature_map}")

  # 3. Instantiate PyTorch KitNET Model
  model = PyTorchKitNET(num_features, feature_map)
  print(
      f"✓ Initialized PyTorch KitNET with {len(model.ensemble_layers)} ensemble"
      " autoencoders."
  )

  # 4. Train Ensemble Layer Autoencoders
  print("\n--- Phase 1: Training Ensemble Layer Autoencoders ---")
  dataset_size = data_tensor.shape[0]
  data_permuted = model.permute_input(data_tensor)

  for idx, ae in enumerate(model.ensemble_layers):
    start, end = model.slice_bounds[idx]
    ae_inputs = data_permuted[:, start:end]

    optimizer = optim.Adam(ae.parameters(), lr=lr)
    criterion = nn.MSELoss()

    ae.train()
    for epoch in range(epochs):
      epoch_loss = 0.0
      num_batches = 0

      for b in range(0, dataset_size, batch_size):
        batch_x = ae_inputs[b : b + batch_size]
        optimizer.zero_grad()

        reconstructed = ae(batch_x)
        loss = criterion(reconstructed, batch_x)

        loss.backward()
        optimizer.step()

        epoch_loss += loss.item()
        num_batches += 1

      avg_loss = epoch_loss / num_batches
      if (epoch + 1) % 2 == 0:
        print(
            f"  Ensemble AE {idx + 1}/{len(model.ensemble_layers)} | Epoch"
            f" [{epoch + 1}/{epochs}] | Loss: {avg_loss:.6e}"
        )

  # 5. Train Output Anomaly Detector Layer
  print("\n--- Phase 2: Training Output Anomaly Detector Layer ---")
  model.eval()
  with torch.no_grad():
    residuals = []
    for idx, ae in enumerate(model.ensemble_layers):
      start, end = model.slice_bounds[idx]
      sub_x = data_permuted[:, start:end]
      sub_reconstructed = ae(sub_x)
      residuals.append(sub_x - sub_reconstructed)
    ensemble_residuals = torch.cat(residuals, dim=1)

  assert model.output_layer is not None
  output_ae = model.output_layer
  ad_optimizer = optim.Adam(output_ae.parameters(), lr=lr)
  ad_criterion = nn.MSELoss()

  output_ae.train()
  for epoch in range(epochs):
    epoch_loss = 0.0
    num_batches = 0

    for b in range(0, dataset_size, batch_size):
      batch_r = ensemble_residuals[b : b + batch_size]
      ad_optimizer.zero_grad()

      reconstructed_r = output_ae(batch_r)
      loss = ad_criterion(reconstructed_r, batch_r)

      loss.backward()
      ad_optimizer.step()

      epoch_loss += loss.item()
      num_batches += 1

    avg_loss = epoch_loss / num_batches
    if (epoch + 1) % 2 == 0:
      print(
          f"  Anomaly Detector AE | Epoch [{epoch + 1}/{epochs}] | Loss:"
          f" {avg_loss:.6e}"
      )

  # 6. Evaluation and Verification
  model.eval()
  with torch.no_grad():
    sse_scores, _ = model(data_tensor)
    mse_scores = (sse_scores / float(num_features)).numpy()

  print("\n" + "=" * 70)
  print("PYTORCH MODEL TRAINING RESULTS")
  print("=" * 70)
  print(f"Average Reconstruction MSE: {np.mean(mse_scores):.6e}")
  print(f"Min Reconstruction MSE:     {np.min(mse_scores):.6e}")
  print(f"Max Reconstruction MSE:     {np.max(mse_scores):.6e}")
  print(f"Std Dev Reconstruction MSE: {np.std(mse_scores):.6e}\n")

  # 7. Save Weights
  resolved_out = Path(resolve_path(output_model_path))
  resolved_out.parent.mkdir(parents=True, exist_ok=True)
  model.save_weights(str(resolved_out))
  print(f"✓ PyTorch model successfully saved to: {resolved_out}")


def main():
  parser = argparse.ArgumentParser(
      description="Train PyTorch KitNET Anomaly Detector."
  )
  parser.add_argument(
      "--dataset",
      type=str,
      choices=["toy", "mini", "full"],
      default="mini",
      help="Dataset size: toy (2 feats), mini (5 feats), or full (50 feats)",
  )
  parser.add_argument(
      "--data-file",
      type=str,
      default="demos/network_anomaly/data/Mirai_first_batch_32K.bin",
      help="Path to binary packet file",
  )
  parser.add_argument(
      "--features", type=int, default=None, help="Override number of features"
  )
  parser.add_argument(
      "--packets",
      type=int,
      default=10000,
      help="Number of packets for training",
  )
  parser.add_argument(
      "--epochs", type=int, default=10, help="Training epochs per layer"
  )
  parser.add_argument("--lr", type=float, default=0.01, help="Learning rate")
  parser.add_argument("--batch-size", type=int, default=64, help="Batch size")
  parser.add_argument(
      "--output",
      type=str,
      default="demos/network_anomaly/data/torch_kitnet_model.pt",
      help="Output model path",
  )
  args = parser.parse_args()

  features = args.features or (
      2 if args.dataset == "toy" else (5 if args.dataset == "mini" else 50)
  )
  train_pytorch_model(
      dataset=args.dataset,
      data_file=args.data_file,
      num_features=features,
      num_packets=args.packets,
      epochs=args.epochs,
      lr=args.lr,
      batch_size=args.batch_size,
      output_model_path=args.output,
  )


if __name__ == "__main__":
  main()
