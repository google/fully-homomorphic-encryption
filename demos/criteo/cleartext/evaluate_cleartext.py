import os

import torch

from demos.common.python import path_utils
from demos.criteo.torch.model import CriteoHELRM

# Dims from criteo_inference_test.py
SPLIT_1_SIZES = [1836] * 12 + [1841]
SPLIT_2_SIZES = [1836] * 12 + [1841]
vocab_sizes = SPLIT_1_SIZES + SPLIT_2_SIZES

model_path = path_utils.resolve_path(
    "demos/criteo/data/criteohelrm.pth"
)

model = CriteoHELRM(vocab_sizes)
checkpoint = torch.load(model_path, map_location="cpu")
state_dict = (
    checkpoint["weights"]
    if isinstance(checkpoint, dict) and "weights" in checkpoint
    else checkpoint
)
state_dict = model.remap_orion_state_dict(state_dict)
model.load_state_dict(state_dict)
model.eval()

# Synthetic inputs matching evaluate_fhe.go
dense = torch.ones(1, 13)
sparse0 = torch.full((1, 23873), 0.5)
sparse1 = torch.full((1, 23873), 0.2)

with torch.no_grad():
  output = model(dense, sparse0, sparse1)

print(f"Output logit: {output.item()}")
