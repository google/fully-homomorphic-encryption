"""A binary for kernel_gaussian_blur."""
from collections.abc import Sequence
import itertools
from typing import List, Tuple

from absl import app
from jaxite.jaxite_bool import bool_params
from jaxite.jaxite_bool import jaxite_bool
from jaxite.jaxite_bool import type_converters
from jaxite.jaxite_lib import random_source
from jaxite.jaxite_lib import types

from transpiler.examples.image_processing import kernel_gaussian_blur_fhe_py_lib

MAX_PIXELS = 8

# pyformat: disable
INPUT_IMAGE = [
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 7, 10, 13, 0, 0, 0, 0,
    0, 7, 10, 13, 0, 0, 0, 0,
    0, 7, 10, 13, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 15, 15, 0,
    0, 0, 0, 0, 0, 15, 15, 0,
    0, 0, 0, 0, 0, 15, 15, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
]

# pyformat: disable
EXPECTED_IMAGE = [
    0, 1, 2, 2, 0, 0, 0, 0,
    1, 4, 7, 6, 2, 0, 0, 0,
    1, 6, 10, 9, 3, 0, 0, 0,
    1, 4, 7, 6, 3, 2, 2, 0,
    0, 1, 2, 2, 3, 8, 8, 2,
    0, 0, 0, 0, 3, 11, 11, 3,
    0, 0, 0, 0, 2, 8, 8, 2,
    0, 0, 0, 0, 0, 2, 2, 0,
]


def get_subset_image(
    padded_image: List[List[types.LweCiphertext]], i: int, j: int
) -> List[List[types.LweCiphertext]]:
  window = [[]] * (3 * 3)
  for iw in range(-1, 2, 1):
    for jw in range(-1, 2, 1):
      window[(iw + 1) * 3 + jw + 1] = padded_image[
          (i + iw + 1) * (MAX_PIXELS + 2) + j + jw + 1
      ]
  return window


# kernel_gaussian_blur acts on the entire input image and calls
# the underlying gaussian blur for each pixel.
def kernel_gaussian_blur(
    my_array: List[List[types.LweCiphertext]],
    sks: jaxite_bool.ServerKeySet,
    params: jaxite_bool.Parameters,
) -> List[List[types.LweCiphertext]]:
  """Apply the Gaussian Blur."""
  res = [[]] * (MAX_PIXELS * MAX_PIXELS)
  for i in range(MAX_PIXELS):
    for j in range(MAX_PIXELS):
      print(f"Processing pixel {(i, j)}")
      subset = get_subset_image(my_array, i, j)
      flatten = list(itertools.chain.from_iterable(subset))
      res[i * MAX_PIXELS + j]: List[types.LweCiphertext] = (
          kernel_gaussian_blur_fhe_py_lib.kernel_gaussian_blur(flatten, sks, params)
      )
  return res


def setup_test_params_keys():
  """Perform necessary setup to generate parameters and keys."""
  print("Generating keys")
  boolean_params = bool_params.get_params_for_test()
  lwe_rng = bool_params.get_rng_for_test(1)
  rlwe_rng = bool_params.get_rng_for_test(1)
  cks = jaxite_bool.ClientKeySet(boolean_params, lwe_rng, rlwe_rng)
  sks = jaxite_bool.ServerKeySet(cks, boolean_params, lwe_rng, rlwe_rng)
  return (boolean_params, lwe_rng, cks, sks)


def setup_input_output(
    cks: jaxite_bool.ClientKeySet, lwe_rng: random_source.PseudorandomSource
) -> List[types.LweCiphertext]:
  """Perform setup for input and output ciphertexts to run benchmark."""
  print("Encrypting input")
  padded_image: List[int] = [0] * ((MAX_PIXELS + 2) * (MAX_PIXELS + 2))
  for i in range(MAX_PIXELS):
    for j in range(MAX_PIXELS):
      padded_image[(i + 1) * (MAX_PIXELS + 2) + j + 1] = INPUT_IMAGE[
          i * MAX_PIXELS + j
      ]

  result: List[List[types.LweCiphertext]] = []
  for i in padded_image:
    bit_slice = type_converters.u8_to_bit_slice(i)
    result.append([jaxite_bool.encrypt(x, cks, lwe_rng) for x in bit_slice])

  return result


def main(argv: Sequence[str]) -> None:
  (boolean_params, lwe_rng, cks, sks) = setup_test_params_keys()
  ciphertext = setup_input_output(cks, lwe_rng)
  result_ciphertext = kernel_gaussian_blur(
      ciphertext, sks, boolean_params
  )

  # Decrypt
  for i, result_slice in enumerate(result_ciphertext):
    bit_slice = [jaxite_bool.decrypt(x, cks) for x in result_slice]
    res = type_converters.bit_slice_to_u8(bit_slice)
    print(res)
    assert res == kernel_gaussian_blur.EXPECTED_IMAGE[i]


if __name__ == '__main__':
  app.run(main)
