"""A jaxite binary for fibonacci."""

from collections.abc import Sequence
import functools

from absl import app
from jaxite.jaxite_bool import bool_params
from jaxite.jaxite_bool import jaxite_bool
from jaxite.jaxite_bool import type_converters
from jaxite.jaxite_lib import types

from transpiler.examples.fibonacci import fibonacci_fhe_py_lib


def bit_slice_to_int(bit_slice: list[bool]) -> int:
  """Given an list of bits, return a base-10 integer."""
  result = 0
  for i, bit in enumerate(bit_slice):
    result |= int(bit) << i
  return result


def int_to_bit_slice(input_int: int, width: int) -> list[bool]:
  """Given an integer and bit width, return a bitwise representation."""
  result: list[bool] = [False] * width
  for i in range(width):
    result[i] = ((input_int >> i) & 1) != 0
  return result


@functools.cache
def setup():
  print(f'Generating keys')
  boolean_params = bool_params.get_params_for_128_bit_security()
  lwe_rng = bool_params.get_lwe_rng_for_128_bit_security(1)
  rlwe_rng = bool_params.get_rlwe_rng_for_128_bit_security(1)
  cks = jaxite_bool.ClientKeySet(boolean_params, lwe_rng, rlwe_rng)
  sks = jaxite_bool.ServerKeySet(cks, boolean_params, lwe_rng, rlwe_rng)
  return (boolean_params, lwe_rng, cks, sks)


def run(cleartext_input):
  (cks, sks, lwe_rng, boolean_params) = setup()
  cleartext_x = int_to_bit_slice(cleartext_input, width=32)
  print(f'Cleartext = {cleartext_x}')
  ciphertext_x = [jaxite_bool.encrypt(z, cks, lwe_rng) for z in cleartext_x]

  print('Running FHE circuit')
  result_ciphertext = fibonacci_fhe_py_lib.fibonacci_number(
      ciphertext_x,
      sks,
      boolean_params,
  )

  result_cleartext = [jaxite_bool.decrypt(z, cks) for z in result_ciphertext]
  print(f'Result cleartext = {result_cleartext}')
  result = bit_slice_to_int(result_cleartext)
  print(f'f({cleartext_input}) = {result}')


def main(argv: Sequence[str]) -> None:
  del argv
  run(1)
  run(2)
  run(3)
  run(4)


if __name__ == '__main__':
  app.run(main)
