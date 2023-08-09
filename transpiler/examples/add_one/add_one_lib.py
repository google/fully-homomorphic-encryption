"""A demonstration of adding 1 to a number in FHE."""
from typing import Any

from jaxite.jaxite_bool import bool_params
from jaxite.jaxite_bool import jaxite_bool
from jaxite.jaxite_bool import type_converters
from jaxite.jaxite_lib import types


def setup(x: int) -> Any:
  """Set up the keys and encryptions for add_one."""
  print('Setting up keys')
  boolean_params = bool_params.get_params_for_128_bit_security()
  lwe_rng = bool_params.get_lwe_rng_for_128_bit_security(1)
  rlwe_rng = bool_params.get_rlwe_rng_for_128_bit_security(1)
  cks = jaxite_bool.ClientKeySet(boolean_params, lwe_rng, rlwe_rng)
  sks = jaxite_bool.ServerKeySet(cks, boolean_params, lwe_rng, rlwe_rng)

  print('Encrypting inputs')
  cleartext_x = type_converters.u8_to_bit_slice(x)
  ciphertext_x = [jaxite_bool.encrypt(z, cks, lwe_rng) for z in cleartext_x]

  return (boolean_params, cks, sks, ciphertext_x)


def decrypt(
    ciphertext: list[types.LweCiphertext], cks: jaxite_bool.ClientKeySet
) -> int:
  """Decrypt the output of add_one."""
  print('Decrypting result')
  return type_converters.bit_slice_to_u8(
      [jaxite_bool.decrypt(z, cks) for z in ciphertext]
  )
