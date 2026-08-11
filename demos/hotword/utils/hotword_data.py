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

"""Speech Commands dataset loader utility using scipy and librosa."""

import os
from pathlib import Path

import numpy as np
import torch
import torch.nn.functional as F
from torch.utils.data import Dataset


KEYWORDS = (
    "yes",
    "no",
    "up",
    "down",
    "left",
    "right",
    "on",
    "off",
    "stop",
    "go",
    "cat",
)
LABELS = ("_silence_", "_unknown_", *KEYWORDS)
SILENCE, UNKNOWN = 0, 1

SAMPLE_RATE = 16000
CLIP_SAMPLES = SAMPLE_RATE  # 1 second


def _load_list(path: Path) -> set[str]:
  if not path.exists():
    return set()
  with open(path, "r") as f:
    return {line.strip() for line in f if line.strip()}


def _get_files(root: Path, subset: str) -> list[Path]:
  all_files = []
  for p in root.glob("*/*.wav"):
    if p.parent.name == "_background_noise_" or p.parent.name.startswith("_"):
      continue
    all_files.append(p)

  if subset == "all":
    return all_files

  val_list = _load_list(root / "validation_list.txt")
  test_list = _load_list(root / "testing_list.txt")

  filtered_files = []
  for p in all_files:
    rel_path = p.relative_to(root).as_posix()
    if subset == "validation":
      if rel_path in val_list:
        filtered_files.append(p)
    elif subset == "testing":
      if rel_path in test_list:
        filtered_files.append(p)
    elif subset == "training":
      if rel_path not in val_list and rel_path not in test_list:
        filtered_files.append(p)
  return filtered_files


def _fix_length(wave: torch.Tensor, n: int) -> torch.Tensor:
  """Center-crop or zero-pad a 1-D waveform to exactly `n` samples."""
  if wave.numel() >= n:
    start = (wave.numel() - n) // 2
    return wave[start : start + n]
  pad = n - wave.numel()
  return F.pad(wave, (pad // 2, pad - pad // 2))


def _label_index(word: str) -> int:
  """Map a Speech Commands folder name to a class index (non-keywords -> unknown)."""
  return LABELS.index(word) if word in KEYWORDS else UNKNOWN


class HotwordDataset(Dataset):
  """Speech Commands dataset loader."""

  def __init__(
      self,
      root: str | None = None,
      npz_path: str | None = None,
      subset: str = "training",
      unknown_silence_frac: float = 0.1,
      seed: int = 0,
      n_mfcc: int = 40,
      n_fft: int = 400,
      hop_length: int = 160,
      n_mels: int = 40,
      clip_samples: int = 16000,
  ):
    self.n_mfcc = n_mfcc
    self.n_fft = n_fft
    self.hop_length = hop_length
    self.n_mels = n_mels
    self.clip_samples = clip_samples

    if npz_path is not None:
      with np.load(npz_path) as data:
        x_key = "x" if "x" in data else "X"
        self.x = torch.from_numpy(data[x_key]).float()
        self.y = torch.from_numpy(data["y"]).long()
      self.mode = "npz"
      return

    self.mode = "raw"
    root_path = Path(root)
    self.files = _get_files(root_path, subset)

    # Split into keywords and unknown
    keyword_files = []
    unknown_files = []
    for p in self.files:
      if p.parent.name in KEYWORDS:
        keyword_files.append(p)
      else:
        unknown_files.append(p)

    # Subsample unknown
    gen = torch.Generator().manual_seed(seed)
    n_extra = max(1, int(len(keyword_files) * unknown_silence_frac))

    if unknown_files:
      perm = torch.randperm(len(unknown_files), generator=gen)
      unknown_files = [unknown_files[i] for i in perm[:n_extra].tolist()]
    else:
      unknown_files = []

    self.files = keyword_files + unknown_files

    # Handle silence
    noise_dir = root_path / "_background_noise_"
    self.noise_waves = []
    if noise_dir.exists():
      from scipy.io import wavfile  # pylint: disable=g-import-not-at-top

      for f in sorted(noise_dir.glob("*.wav")):
        sr, wave = wavfile.read(str(f))
        if sr != SAMPLE_RATE:
          raise ValueError(
              f"Expected sample rate {SAMPLE_RATE}, but got {sr} for {f}"
          )
        wave_dtype = wave.dtype
        wave_tensor = torch.from_numpy(wave).float()
        if wave_dtype == np.int16:
          wave_tensor = wave_tensor / 32768.0
        elif wave_dtype == np.int32:
          wave_tensor = wave_tensor / 2147483648.0
        if wave_tensor.ndim > 1:
          wave_tensor = wave_tensor.mean(dim=-1)
        self.noise_waves.append(wave_tensor)

    self.silence_plan = []
    if self.noise_waves:
      for _ in range(n_extra):
        i = int(torch.randint(len(self.noise_waves), (1,), generator=gen))
        room = max(1, self.noise_waves[i].numel() - self.clip_samples)
        offset = int(torch.randint(room, (1,), generator=gen))
        gain = float(torch.rand(1, generator=gen)) * 0.5
        self.silence_plan.append((i, offset, gain))

  def __len__(self) -> int:
    if self.mode == "npz":
      return len(self.x)
    return len(self.files) + len(self.silence_plan)

  def __getitem__(self, idx: int) -> tuple[torch.Tensor, int]:
    if self.mode == "npz":
      return self.x[idx], self.y[idx]

    if idx < len(self.files):
      from scipy.io import wavfile  # pylint: disable=g-import-not-at-top

      path = self.files[idx]
      sr, wave = wavfile.read(str(path))
      if sr != SAMPLE_RATE:
        raise ValueError(
            f"Expected sample rate {SAMPLE_RATE}, but got {sr} for {path}"
        )
      wave_dtype = wave.dtype
      wave_tensor = torch.from_numpy(wave).float()
      if wave_dtype == np.int16:
        wave_tensor = wave_tensor / 32768.0
      elif wave_dtype == np.int32:
        wave_tensor = wave_tensor / 2147483648.0
      if wave_tensor.ndim > 1:
        wave_tensor = wave_tensor.mean(dim=-1)
      wave = wave_tensor
      label = _label_index(path.parent.name)
    else:
      silence_idx = idx - len(self.files)
      i, offset, gain = self.silence_plan[silence_idx]
      clip = self.noise_waves[i][offset : offset + self.clip_samples]
      wave = _fix_length(clip, self.clip_samples) * gain
      label = SILENCE

    wave_fixed = _fix_length(wave, self.clip_samples)
    wave_np = wave_fixed.numpy()

    # Extract MFCC
    import librosa  # pylint: disable=g-import-not-at-top

    mfcc_np = librosa.feature.mfcc(
        y=wave_np,
        sr=SAMPLE_RATE,
        n_mfcc=self.n_mfcc,
        n_fft=self.n_fft,
        hop_length=self.hop_length,
        n_mels=self.n_mels,
        htk=True,
    )
    x = torch.from_numpy(mfcc_np).float()
    return x, label
