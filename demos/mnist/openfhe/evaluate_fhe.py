"""OpenFHE evaluation runner for a single MNIST sample."""

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
    "sample_idx", 0, "Index of the MNIST sample to evaluate (0-9999)"
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


def evaluate_single(data_dir: str, sample_idx: int) -> None:
  """Runs OpenFHE homomorphic evaluation for a single MNIST sample."""
  t0 = time.perf_counter()

  image, label = load_mnist_sample(data_dir, sample_idx)
  input_vector = image.flatten().tolist()

  t2 = time.perf_counter()
  crypto_context = mnist.mnist__generate_crypto_context()
  key_pair = crypto_context.KeyGen()
  public_key = key_pair.publicKey
  secret_key = key_pair.secretKey
  crypto_context = mnist.mnist__configure_crypto_context(
      crypto_context, secret_key
  )
  t3 = time.perf_counter()
  crypto_setup_ms = (t3 - t2) * 1000.0

  zero_encrypt_func_names = sorted(
      [name for name in dir(mnist) if name.startswith("mnist__encrypt__zero__")]
  )
  zero_encrypt_funcs = [
      getattr(mnist, name) for name in zero_encrypt_func_names
  ]

  t4 = time.perf_counter()
  input_encrypted = mnist.mnist__encrypt__arg0(
      crypto_context, input_vector, public_key
  )
  ct_zeros = [func(crypto_context, public_key) for func in zero_encrypt_funcs]
  t5 = time.perf_counter()
  encrypt_ms = (t5 - t4) * 1000.0

  t6 = time.perf_counter()
  output_encrypted = mnist.mnist(crypto_context, input_encrypted, *ct_zeros)
  t7 = time.perf_counter()
  eval_ms = (t7 - t6) * 1000.0

  t8 = time.perf_counter()
  output = mnist.mnist__decrypt__result0(
      crypto_context, output_encrypted, secret_key
  )
  t9 = time.perf_counter()
  decrypt_ms = (t9 - t8) * 1000.0

  logits = output[:10]
  pred = max(range(10), key=lambda i: logits[i])
  is_correct = pred == label
  total_ms = (t9 - t0) * 1000.0

  print(f"\nEvaluating MNIST Sample Index: {sample_idx}")
  print("--- Detailed Step Latencies ---")
  print(f"Crypto Setup & KeyGen:   {crypto_setup_ms:10.4f} ms")
  print(f"Input & Zero Encryption: {encrypt_ms:10.4f} ms")
  print(f"Homomorphic Evaluation:  {eval_ms:10.4f} ms")
  print(f"Decryption:              {decrypt_ms:10.4f} ms")
  print(f"Total Latency:           {total_ms:10.4f} ms")
  print("-------------------------------")
  print(f"True Label:      {label}")
  print(f"Predicted Label: {pred}")
  print(f"Result:          {'CORRECT' if is_correct else 'INCORRECT'}")


def main(argv):
  if len(argv) > 1:
    raise app.UsageError("Too many command-line arguments.")
  try:
    evaluate_single(FLAGS.data_dir, FLAGS.sample_idx)
  except Exception as e:
    print(f"Error evaluating sample {FLAGS.sample_idx}: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  app.run(main)
