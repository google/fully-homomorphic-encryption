import timeit
import os
import pickle

from jaxite.jaxite_bool import bool_params
from jaxite.jaxite_bool import jaxite_bool
from jaxite.jaxite_bool import type_converters
import add_one_lut3_fhe_lib

client_key_set_path = "client_key_set.pkl"
server_key_set_path = "server_key_set.pkl"

# Note: In real applications, a cryptographically secure seed needs to be
# used.
lwe_rng = bool_params.get_lwe_rng_for_128_bit_security(seed=1)
rlwe_rng = bool_params.get_rlwe_rng_for_128_bit_security(seed=1)
params = bool_params.get_params_for_128_bit_security()

print("Using parameters: ", params)
print("Generate or load client and server key sets")
if os.path.exists(client_key_set_path) and os.path.exists(server_key_set_path):
  with open(client_key_set_path, "rb") as f:
    cks = pickle.load(f)
  with open(server_key_set_path, "rb") as f:
    sks = pickle.load(f)
  print("Client key set and Server key set loaded from file.")
else:
  cks = jaxite_bool.ClientKeySet(
      params,
      lwe_rng=lwe_rng,
      rlwe_rng=rlwe_rng,
  )
  with open(client_key_set_path, "wb") as f:
    pickle.dump(cks, f)
  print("Client key set generated and saved to file.")

  sks = jaxite_bool.ServerKeySet(
      cks,
      params,
      lwe_rng=lwe_rng,
      rlwe_rng=rlwe_rng,
      bootstrap_callback=None,
  )
  with open("server_key_set.pkl", "wb") as f:
    pickle.dump(sks, f)
  print("Server key set generated and saved to file.")


x = 5
print("Encrypting input: ", x)
cleartext_x = type_converters.u8_to_bit_slice(x)
ciphertext_x = [jaxite_bool.encrypt(z, cks, lwe_rng) for z in cleartext_x]
print("Done encrypting input")

print("Running add_one_lut3 on TPU")
print(
    "Calling function once before timing it so compile-time doesn't get"
    " included in timing metrics."
)
result_ciphertext = add_one_lut3_fhe_lib.add_one_lut3(ciphertext_x, sks, params)

print("Running benchmark for add_one_lut3 on TPU")


# Using Timeit
def timed_fn():
  result_ciphertext = add_one_lut3_fhe_lib.add_one_lut3(ciphertext_x, sks, params)
  for c in result_ciphertext:
    c.block_until_ready()


timer = timeit.Timer(timed_fn)
execution_time = timer.repeat(repeat=1, number=1)
print("add_one_lut3 execution time: ", execution_time)

expected = x + 1
print("Decrypting result")
actual = type_converters.bit_slice_to_u8(
    [jaxite_bool.decrypt(z, cks) for z in result_ciphertext]
)
print(f"{actual=}, {expected=}")
