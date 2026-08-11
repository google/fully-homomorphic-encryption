from typing import Annotated

import torch
import torch.nn as nn


class Secret:
  pass


class ToyHELRM(nn.Module):
  """HEIR-friendly variant of the *tiny* HE-LRM model (4 dense + 9 one-hot

  sparse = 13 input, hidden_dim=2). The Criteo-shaped real model is
  `CriteoHELRM`; this one stays as the small smoke test for the HEIR path.

  Dims mirror the orion ToyHELRM (`orion.models.helrm.HELRM`) exactly,
  so the bundled `<orion>/model_state/toyhelrm.pth` loads via
  `remap_orion_state_dict` with all 13/13 tensors. The only structural
  difference is single-arg input + masked Linear in place of
  `on.Extract` / `on.ExtractSparse` — `tensor.extract_slice` /
  `tensor.concat` aren't supported on the pytorch-heir-lattigo path
  (heir-opt segfaults), so we mask zeros into the unwanted input cols
  instead of slicing.
  """

  def __init__(self, dense_size=4, vocab_sizes=None, hidden_dim=2):
    super(ToyHELRM, self).__init__()
    if vocab_sizes is None:
      vocab_sizes = [4, 3, 2]

    self.dense_size = dense_size
    self.sparse_size = sum(vocab_sizes)
    self.hidden_dim = hidden_dim
    self.total_size = self.dense_size + self.sparse_size
    self.num_top_inp = self.hidden_dim * (1 + len(vocab_sizes))

    # Mirror orion ToyHELRM dims (`orion.models.helrm.HELRM`):
    #   bot_l: Extract(13->4) -> Linear(4,3) -> ReLU -> Linear(3,2)
    #          -> ReLU -> Linear(2, num_top_inp=8)
    #   embs:  ExtractSparse(13->9) -> Embedding(9, 6) -> Linear(6, 8)
    # Both branches end with a Linear that *expands* its input
    # (2->8 and 6->8). That used to fail downstream of
    # `--torch-linalg-to-ckks`: HEIR pre-folds the two laid-out biases
    # via `arith.addf` in `@main__preprocessing`, and one-shot
    # bufferization didn't handle bare `arith.addf` on tensors. Fixed
    # in heir by inserting `--convert-elementwise-to-linalg` before
    # `oneShotBufferize` in `toLattigoPipelineBuilder` (see
    # `lib/Pipelines/ArithmeticPipelineRegistration.cpp`).
    # NOTE: the last Linear of each branch is `bias=False` to match
    # orion's `generate_concat_transforms` (which creates an
    # `on.Linear(.., .., bias=False)` set to an identity-pad pattern).
    # If it were `bias=True` here, the random-init bias would leak
    # into the merge-add and break numerical agreement with orion.
    self.bot_l = nn.Sequential(
        self._masked_linear(
            self.total_size,
            3,
            in_start=0,
            in_end=self.dense_size,
        ),
        nn.ReLU(),
        nn.Linear(3, self.hidden_dim),
        nn.ReLU(),
        nn.Linear(self.hidden_dim, self.num_top_inp, bias=False),
    )
    self.emb_l = nn.Sequential(
        self._masked_linear(
            self.total_size,
            self.hidden_dim * len(vocab_sizes),
            in_start=self.dense_size,
            in_end=self.total_size,
            bias=False,
        ),
        nn.Linear(
            self.hidden_dim * len(vocab_sizes),
            self.num_top_inp,
            bias=False,
        ),
    )

    # Top MLP - matches orion exactly, including the `Linear(2, 1)`
    # logit head. (Used to need a `Linear(2, 2)` workaround because
    # `Linear(.., 1)` made heir-opt --torch-linalg-to-ckks emit invalid
    # `tensor.extract` IR; that's now fixed in heir.)
    self.top_l = nn.Sequential(
        nn.Linear(self.num_top_inp, 4),
        nn.ReLU(),
        nn.Linear(4, 2),
        nn.ReLU(),
        nn.Linear(2, 1),
        # nn.Sigmoid()
    )

  def forward(self, x: Annotated[torch.Tensor, Secret]) -> torch.Tensor:
    dense_out = self.bot_l(x)
    sparse_out = self.emb_l(x)
    interact = dense_out + sparse_out
    return self.top_l(interact)

  @staticmethod
  def _masked_linear(in_features, out_features, in_start, in_end, bias=True):
    """Linear that zeros out the input columns outside `[in_start, in_end)`,

    used in place of `on.Extract` to slice a concatenated input vector
    without hitting `tensor.extract_slice` (unsupported on pytorch-lattigo).
    """
    lin = nn.Linear(in_features, out_features, bias=bias)
    with torch.no_grad():
      mask = torch.zeros(in_features)
      mask[in_start:in_end] = 1.0
      lin.weight.mul_(mask)
    return lin

  def remap_orion_state_dict(self, orion_state: dict) -> dict:
    """Adapt an `orion.models.helrm.HELRM` state_dict to this model.

    Two structural deltas:

    1. **Index shift in `bot_l` / `embs`.** orion places `on.Extract`
       at `bot_l[0]` (no params), so its first Linear is `bot_l.1`.
       Plaintext collapses the slicing into the first masked Linear,
       so its first Linear is `bot_l.0`. Same for `embs.{i}` →
       `emb_l.{i-1}`.

    2. **First-Linear input width.** orion's `bot_l.1` is
       `Linear(dense_size, 3)`; plaintext's `bot_l.0` is masked
       `Linear(total_size, 3)`. The orion weight goes into the first
       `dense_size` columns of the wider plaintext weight, rest stay
       zero (the mask). Same idea for `embs.1` → `emb_l.0` over
       sparse columns.
    """
    target_state = self.state_dict()
    out: dict = {}

    for k, v in orion_state.items():
      if k.startswith("bot_l."):
        _, idx, kind = k.split(".", 2)
        new_idx = int(idx) - 1
        if new_idx < 0:
          continue  # bot_l.0 is on.Extract, no params
        new_key = f"bot_l.{new_idx}.{kind}"
        if new_idx == 0 and kind == "weight":
          target = target_state[new_key].clone().zero_()
          target[:, : self.dense_size] = v
          out[new_key] = target
        else:
          out[new_key] = v
      elif k.startswith("embs."):
        _, idx, kind = k.split(".", 2)
        new_idx = int(idx) - 1
        if new_idx < 0:
          continue  # embs.0 is on.ExtractSparse, no params
        new_key = f"emb_l.{new_idx}.{kind}"
        if new_idx == 0 and kind == "weight":
          target = target_state[new_key].clone().zero_()
          target[:, self.dense_size :] = v
          out[new_key] = target
        else:
          out[new_key] = v
      else:
        out[k] = v
    return out


def _criteo_mlp(widths, last_relu):
  """nn.* mirror of orion.models.criteo_helrm._mlp."""
  layers: list[nn.Module] = []
  for index, (left, right) in enumerate(zip(widths, widths[1:])):
    layers.append(nn.Linear(left, right))
    if last_relu or index < len(widths) - 2:
      layers.append(nn.ReLU())
  return nn.Sequential(*layers)


def _criteo_copy_into(width, output_width, start):
  """nn.* mirror of orion.models.criteo_helrm._copy_into: a fixed identity-pad

  Linear scattering `width` features into a wider zero vector at `start`.
  """
  layer = nn.Linear(width, output_width, bias=False)
  layer.weight.data[:] = 0.0
  layer.weight.data[start : start + width, :width] = torch.eye(width)
  return layer


def _even_blocks(width: int, max_block: int) -> list[tuple[int, int]]:
  """Split `width` into the fewest near-equal (offset, size) blocks each

  <= max_block. 47746 -> [(0,23873),(23873,23873)].
  """
  nblk = max(1, -(-width // max_block))  # ceil
  base, rem = divmod(width, nblk)
  bounds, off = [], 0
  for i in range(nblk):
    w = base + (1 if i < rem else 0)
    bounds.append((off, w))
    off += w
  return bounds


class CriteoHELRM(nn.Module):
  """Plaintext counterpart of `orion.models.criteo_helrm.CriteoHELRM`.

  Same MLP widths and connectivity. The sparse embedding — orion's
  `on.Embedding(sparse_width, sparse_output)`, a bias-free matvec over the
  expanded (one-hot / radix) sparse vector — is **block-decomposed along its
  contraction dim** into <=`_MAX_BLOCK`-wide pieces (`sparse_blocks`). For the
  real Criteo config sparse_width=47746 splits into 2 blocks of 23873, so no
  single matmul (nor input ciphertext) exceeds 32768 slots and HEIR stays at
  logN16. The block-sum `sum_i block_i(chunk_i)` is exactly the full matvec.

  To keep each block's input within one ciphertext, the expanded-sparse input
  is also split (`split_inputs`) into one chunk per block — fed as separate
  ciphertext args. `forward` accepts both the split form (export path, one
  Secret arg per block) and the unsplit form (cleartext path, the whole
  vector in `sparse0`), reslicing in the latter case.

  Submodule names mirror orion's (`bottom`, `bottom_to_top`, `sparse_to_top`,
  `top`); the wide `sparse` becomes `sparse_blocks` and a checkpoint's
  `sparse.weight` is sliced into the blocks on load.
  """

  # Largest matrix dim (and input-ciphertext width) HEIR's diagonal-layout pass
  # will tolerate before it inflates the ring. 32768 = logN16 slots; one more and
  # the embedding's 47746-wide matmul would force logN17 (2x memory, and the
  # logN17 path is numerically unverified — see the cheddar boot/scale notes).
  _MAX_BLOCK = 32768

  def __init__(self, sparse_sizes=None, compress_threshold=20000, base=4):
    super().__init__()
    dense_features = 13
    sparse_fields = 26
    embedding_features = 16
    bottom_widths = [dense_features, 512, 256, 64, embedding_features]
    top_widths = [embedding_features * (sparse_fields + 1), 512, 256, 1]

    import math

    def radix_width(categories, base):
      return math.ceil(math.log(categories, base))

    def _expanded_width(categories, compress_threshold, base):
      if categories > compress_threshold:
        return base * radix_width(categories, base)
      return categories

    if sparse_sizes is None:
      raise ValueError("sparse_sizes must be provided")

    self.sparse_sizes = list(sparse_sizes)
    self.compress_threshold = compress_threshold
    self.base = base
    self.sparse_width = sum(
        _expanded_width(size, compress_threshold, base)
        for size in self.sparse_sizes
    )
    self.total_size = dense_features + self.sparse_width

    top_input = top_widths[0]
    sparse_output = sparse_fields * embedding_features

    self.bottom = _criteo_mlp(bottom_widths, last_relu=True)
    self.bottom_to_top = _criteo_copy_into(embedding_features, top_input, 0)
    self.sparse_bounds = _even_blocks(self.sparse_width, self._MAX_BLOCK)
    # forward() takes one explicit Secret arg per block (variadic *args
    # wouldn't get {secret.secret}, which keys off parameter name); the
    # 2-arg signature covers sparse_width <= 2*_MAX_BLOCK (Criteo = 47746).
    assert len(self.sparse_bounds) <= 2, (
        f"sparse_width={self.sparse_width} needs {len(self.sparse_bounds)} "
        "blocks; forward() only wires 2 (raise _MAX_BLOCK or extend forward)"
    )
    self.sparse_blocks = nn.ModuleList(
        nn.Linear(w, sparse_output, bias=False) for _, w in self.sparse_bounds
    )
    self.sparse_to_top = _criteo_copy_into(
        sparse_output, top_input, embedding_features
    )
    self.top = _criteo_mlp(top_widths, last_relu=False)

  def split_inputs(self, inputs):
    """(dense, expanded_sparse) -> (dense, *per-block sparse chunks).

    The export pipeline feeds each returned tensor as its own ciphertext.
    """
    dense, expanded_sparse = inputs
    chunks = [
        expanded_sparse[..., off : off + w] for off, w in self.sparse_bounds
    ]
    return (dense, *chunks)

  def _sparse_embed(self, sparse0, sparse1):
    if sparse1 is None and sparse0.shape[-1] == self.sparse_width:
      # Unsplit (cleartext) path: reslice the full vector at block bounds.
      parts = [sparse0[..., off : off + w] for off, w in self.sparse_bounds]
    else:
      parts = [p for p in (sparse0, sparse1) if p is not None]
    out = None
    for block, part in zip(self.sparse_blocks, parts):
      contribution = block(part)
      out = contribution if out is None else out + contribution
    return out

  def forward(
      self,
      dense: Annotated[torch.Tensor, Secret],
      sparse0: Annotated[torch.Tensor, Secret],
      sparse1: Annotated[torch.Tensor, Secret] = None,
  ) -> torch.Tensor:
    dense = self.bottom_to_top(self.bottom(dense))
    sparse = self.sparse_to_top(self._sparse_embed(sparse0, sparse1))
    return self.top(dense + sparse)

  def remap_orion_state_dict(self, state):
    # Checkpoints store the embedding as a single `sparse.weight`
    # (sparse_output, sparse_width); slice it column-wise into the blocks.
    # `load_pretrained` calls this before its shape-compatibility filter, so
    # the sliced block keys land in the model instead of being skipped.
    if "sparse.weight" not in state:
      return state
    W = state["sparse.weight"]
    state = {k: v for k, v in state.items() if k != "sparse.weight"}
    for i, (off, w) in enumerate(self.sparse_bounds):
      state[f"sparse_blocks.{i}.weight"] = W[:, off : off + w]
    return state
