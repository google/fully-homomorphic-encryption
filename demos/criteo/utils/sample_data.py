import argparse
import hashlib
import os
import torch

SPLIT_1_SIZES = [1836] * 12 + [1841]
SPLIT_2_SIZES = [1836] * 12 + [1841]

SPLIT_1_TOTAL = sum(SPLIT_1_SIZES)
SPLIT_2_TOTAL = sum(SPLIT_2_SIZES)


def stable_hash(val: str, bucket_size: int) -> int:
  if not val:
    return 0
  return int(hashlib.md5(val.encode('utf-8')).hexdigest(), 16) % bucket_size


def preprocess_line(line, delimiter='\t'):
  parts = line.strip().split(delimiter)
  if len(parts) < 40:
    return None

  label = float(parts[0])

  # Dense
  dense_vals = []
  for i in range(1, 14):
    val_str = parts[i]
    if not val_str:
      val = 0.0
    else:
      val = float(val_str)
      if val > 0:
        val = torch.log(torch.tensor(val) + 1.0).item()
      else:
        val = 0.0
    dense_vals.append(val)
  dense_tensor = torch.tensor(dense_vals, dtype=torch.float32)

  # Sparse 1
  sparse_x1 = torch.zeros(SPLIT_1_TOTAL)
  offset = 0
  for i in range(13):
    val_str = parts[14 + i]
    active_idx = stable_hash(val_str, SPLIT_1_SIZES[i])
    sparse_x1[offset + active_idx] = 1.0
    offset += SPLIT_1_SIZES[i]

  # Sparse 2
  sparse_x2 = torch.zeros(SPLIT_2_TOTAL)
  offset = 0
  for i in range(13):
    val_str = parts[27 + i]
    active_idx = stable_hash(val_str, SPLIT_2_SIZES[i])
    sparse_x2[offset + active_idx] = 1.0
    offset += SPLIT_2_SIZES[i]

  return dense_tensor, sparse_x1, sparse_x2, label


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--raw_data', type=str, required=True, help='Path to raw train.txt'
  )
  parser.add_argument(
      '--output', type=str, required=True, help='Path to save sample.pt'
  )
  parser.add_argument(
      '--num_samples', type=int, default=20, help='Number of samples to get'
  )
  args = parser.parse_args()

  if not os.path.exists(args.raw_data):
    print(f'Error: {args.raw_data} does not exist.')
    return

  dense_list = []
  sparse_x1_list = []
  sparse_x2_list = []
  label_list = []

  count = 0
  with open(args.raw_data, 'r') as f:
    for line in f:
      res = preprocess_line(line)
      if res is None:
        continue
      dense, sx1, sx2, lbl = res
      dense_list.append(dense)
      sparse_x1_list.append(sx1)
      sparse_x2_list.append(sx2)
      label_list.append(lbl)
      count += 1
      if count >= args.num_samples:
        break

  if count < args.num_samples:
    print(
        f'Warning: Only found {count} valid samples (requested'
        f' {args.num_samples})'
    )

  if count == 0:
    print('Error: No valid samples found.')
    return

  sample = {
      'dense': torch.stack(dense_list),
      'sparse_x1': torch.stack(sparse_x1_list),
      'sparse_x2': torch.stack(sparse_x2_list),
      'labels': torch.tensor(label_list, dtype=torch.float32).unsqueeze(1),
  }

  out_dir = os.path.dirname(args.output)
  if out_dir:
    os.makedirs(out_dir, exist_ok=True)

  torch.save(sample, args.output)
  print(f'Saved {count} samples to {args.output}')


if __name__ == '__main__':
  main()
