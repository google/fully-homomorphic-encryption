"""OpenFHE evaluation suite runner over MNIST test dataset."""

import os
import sys
import time

from absl import app
from absl import flags
import numpy as np

from demos.common.python import path_utils

try:
  from demos.mnist.openfhe import mnist_openfhe_pybind as mnist
except ImportError:
  try:
    from demos.mnist.openfhe import mnist_openfhe_pybind as mnist
  except ImportError:
    import mnist_openfhe_pybind as mnist

try:
  from demos.mnist.utils.mnist_data import MnistDataset
except ImportError:
  try:
    from demos.mnist.utils.mnist_data import MnistDataset
  except ImportError:
    from mnist_data import MnistDataset

resolve_path = path_utils.resolve_path

FLAGS = flags.FLAGS
flags.DEFINE_integer(
    "num_samples",
    5,
    "Number of test samples to evaluate sequentially (default 5)",
)
flags.DEFINE_string(
    "data_dir",
    "demos/mnist/data",
    "Directory containing MNIST dataset binary files",
)


def load_mnist_sample(data_dir: str, sample_idx: int) -> tuple[np.ndarray, int]:
  """Loads and normalizes a single image and label from MNIST NPZ file."""
  npz_path = resolve_path(os.path.join(data_dir, "mnist.npz"))
  dataset = MnistDataset(npz_path, reshape_to_2d=False)
  image, label = dataset[sample_idx]
  return image.flatten(), label


def evaluate_suite(data_dir: str, num_samples: int) -> None:
  """Evaluates OpenFHE model sequentially over multiple MNIST samples."""

  print("Configuring OpenFHE crypto context...")
  t_setup_start = time.perf_counter()
  crypto_context = mnist.mnist__generate_crypto_context()
  key_pair = crypto_context.KeyGen()
  public_key = key_pair.publicKey
  secret_key = key_pair.secretKey
  crypto_context = mnist.mnist__configure_crypto_context(
      crypto_context, secret_key
  )
  t_setup_end = time.perf_counter()
  print(
      "Crypto context setup completed in"
      f" {(t_setup_end - t_setup_start)*1000:.2f} ms.\n"
  )

  zero_encrypt_func_names = sorted(
      [name for name in dir(mnist) if name.startswith("mnist__encrypt__zero__")]
  )
  zero_encrypt_funcs = [
      getattr(mnist, name) for name in zero_encrypt_func_names
  ]
  ct_zeros = [func(crypto_context, public_key) for func in zero_encrypt_funcs]

  print(f"Evaluating {num_samples} MNIST samples sequentially...")
  correct = 0
  total_eval_time_s = 0.0

  for i in range(num_samples):
    image, label = load_mnist_sample(data_dir, i)
    input_vector = image.flatten().tolist()

    input_encrypted = mnist.mnist__encrypt__arg0(
        crypto_context, input_vector, public_key
    )

    t_eval_start = time.perf_counter()
    output_encrypted = mnist.mnist(crypto_context, input_encrypted, *ct_zeros)
    t_eval_end = time.perf_counter()

    eval_time_s = t_eval_end - t_eval_start
    total_eval_time_s += eval_time_s

    output = mnist.mnist__decrypt__result0(
        crypto_context, output_encrypted, secret_key
    )
    logits = output[:10]
    pred = max(range(10), key=lambda idx: logits[idx])
    is_correct = pred == label
    if is_correct:
      correct += 1

    print(
        f"Sample {i:4d}: True={label}, Pred={pred} |"
        f" {'CORRECT' if is_correct else 'INCORRECT'} | Eval Time="
        f" {eval_time_s*1000:.2f} ms"
    )

  accuracy = (correct / num_samples) * 100.0 if num_samples > 0 else 0.0
  avg_eval_ms = (
      (total_eval_time_s / num_samples) * 1000.0 if num_samples > 0 else 0.0
  )

  print("\n--- OpenFHE Evaluation Suite Results ---")
  print(f"Total Samples Evaluated: {num_samples}")
  print(f"Total Correct:           {correct} / {num_samples}")
  print(f"Overall Accuracy:        {accuracy:.2f}%")
  print(f"Average Homomorphic Eval Latency: {avg_eval_ms:.2f} ms/sample")


def main(argv):
  if len(argv) > 1:
    raise app.UsageError("Too many command-line arguments.")
  try:
    evaluate_suite(FLAGS.data_dir, FLAGS.num_samples)
  except Exception as e:
    print(f"Error executing evaluation suite: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  app.run(main)
