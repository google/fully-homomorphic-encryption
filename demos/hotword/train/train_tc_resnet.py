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

"""Train TC-ResNet on Google Speech Commands (keyword spotting)."""

import argparse
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader

from demos.hotword.torch import model as tc_resnet
from demos.hotword.utils import hotword_data
from demos.hotword.utils.hotword_data import HotwordDataset

# Use same labels as in hotword_data
LABELS = hotword_data.LABELS


def _export_test_data(dataset: HotwordDataset, out_path: str, n: int) -> None:
  n = min(n, len(dataset))
  xs, ys = [], []
  for i in range(n):
    x, y = dataset[i]
    xs.append(x)
    ys.append(y)

  Path(out_path).parent.mkdir(parents=True, exist_ok=True)
  # Save with 'x' and 'y' keys
  np.savez(
      out_path,
      x=torch.stack(xs).numpy().astype(np.float32),
      y=np.asarray(ys, dtype=np.int64),
  )
  print(f"Saved {n} test clips to {out_path}")


def main() -> None:
  ap = argparse.ArgumentParser()
  ap.add_argument(
      "--model", choices=["tc_resnet8", "tc_resnet14"], default="tc_resnet8"
  )
  ap.add_argument(
      "--variant",
      choices=["small", "large"],
      default="small",
      help="Which variant to train (small or large). Defines MFCC parameters.",
  )
  ap.add_argument("--epochs", type=int, default=30)
  ap.add_argument("--batch-size", type=int, default=128)
  ap.add_argument("--lr", type=float, default=1e-3)
  ap.add_argument("--weight-decay", type=float, default=1e-4)
  ap.add_argument("--num-workers", type=int, default=0)
  ap.add_argument(
      "--test-samples",
      type=int,
      default=1000,
      help="Number of test clips to export.",
  )
  ap.add_argument("--seed", type=int, default=42)
  ap.add_argument(
      "--device",
      type=str,
      default="cuda" if torch.cuda.is_available() else "cpu",
      help="Device to use for training (cuda or cpu)",
  )
  ap.add_argument(
      "--data_dir",
      type=str,
      required=True,
      help="Path to Speech Commands dataset directory.",
  )
  ap.add_argument(
      "--out_model",
      type=str,
      required=True,
      help="Path to save the trained model (.pth).",
  )
  ap.add_argument(
      "--out_test_data",
      type=str,
      required=True,
      help="Path to save the exported test data (.npz).",
  )
  args = ap.parse_args()

  torch.manual_seed(args.seed)
  device = torch.device(args.device)

  if args.variant == "small":
    n_mfcc = 10
    n_fft = 640
    hop_length = 320
    clip_samples = 15680
  else:
    n_mfcc = 40
    n_fft = 400
    hop_length = 160
    clip_samples = 16000

  print(f"Loading data from {args.data_dir} ({args.variant} variant)...")
  train_ds = HotwordDataset(
      root=args.data_dir,
      subset="training",
      seed=args.seed,
      n_mfcc=n_mfcc,
      n_fft=n_fft,
      hop_length=hop_length,
      clip_samples=clip_samples,
  )
  val_ds = HotwordDataset(
      root=args.data_dir,
      subset="validation",
      seed=args.seed,
      n_mfcc=n_mfcc,
      n_fft=n_fft,
      hop_length=hop_length,
      clip_samples=clip_samples,
  )
  test_ds = HotwordDataset(
      root=args.data_dir,
      subset="testing",
      seed=args.seed,
      n_mfcc=n_mfcc,
      n_fft=n_fft,
      hop_length=hop_length,
      clip_samples=clip_samples,
  )

  train_loader = DataLoader(
      train_ds,
      batch_size=args.batch_size,
      shuffle=True,
      num_workers=args.num_workers,
  )
  val_loader = DataLoader(
      val_ds, batch_size=args.batch_size, num_workers=args.num_workers
  )

  model = getattr(tc_resnet, args.model)(
      n_mfcc=n_mfcc, num_classes=len(LABELS)
  ).to(device)

  opt = optim.Adam(
      model.parameters(), lr=args.lr, weight_decay=args.weight_decay
  )
  sched = optim.lr_scheduler.CosineAnnealingLR(opt, T_max=args.epochs)
  loss_fn = nn.CrossEntropyLoss()

  best_acc, best_state = 0.0, None
  for epoch in range(args.epochs):
    model.train()
    running = 0.0
    for x, y in train_loader:
      x, y = x.to(device), y.to(device)
      opt.zero_grad()
      loss = loss_fn(model(x), y)
      loss.backward()
      opt.step()
      running += loss.item()
    sched.step()

    model.eval()
    correct = total = 0
    with torch.no_grad():
      for x, y in val_loader:
        x, y = x.to(device), y.to(device)
        correct += (model(x).argmax(1) == y).sum().item()
        total += y.size(0)
    val_acc = correct / total
    print(
        f"epoch {epoch + 1}/{args.epochs}  lr={sched.get_last_lr()[0]:.5f}  "
        f"train_loss={running / len(train_loader):.3f}  val_acc={val_acc:.4f}"
    )

    if val_acc > best_acc:
      best_acc = val_acc
      best_state = {
          k: v.detach().cpu().clone() for k, v in model.state_dict().items()
      }

  print(f"best val acc: {best_acc:.4f}")

  # Save best model
  Path(args.out_model).parent.mkdir(parents=True, exist_ok=True)
  torch.save(best_state, args.out_model)
  print(f"saved state_dict to {args.out_model}")

  # Export test data
  _export_test_data(test_ds, args.out_test_data, args.test_samples)


if __name__ == "__main__":
  main()
