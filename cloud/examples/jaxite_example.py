"""Example script showing how to use Jaxite."""

import os
import timeit
import pickle
from jaxite.jaxite_bool import jaxite_bool


bool_params = jaxite_bool.bool_params
client_key_set_path = "client_key_set.pkl"
server_key_set_path = "server_key_set.pkl"

# Note: In real applications, a cryptographically secure seed needs to be
# used.
lwe_rng = bool_params.get_lwe_rng_for_128_bit_security(seed=1)
rlwe_rng = bool_params.get_rlwe_rng_for_128_bit_security(seed=1)
params = bool_params.get_params_for_128_bit_security()

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

ct_true = jaxite_bool.encrypt(True, cks, lwe_rng)
ct_false = jaxite_bool.encrypt(False, cks, lwe_rng)

# Calling function once before timing it so compile-time doesn't get
# included in timing metircs.
and_gate = jaxite_bool.and_(ct_false, ct_true, sks, params)


# Using Timeit
def timed_fn():
  and_gate = jaxite_bool.and_(ct_false, ct_true, sks, params)
  and_gate.block_until_ready()


timer = timeit.Timer(timed_fn)
execution_time = timer.repeat(repeat=1, number=1)
print("And gate execution time: ", execution_time)

actual = jaxite_bool.decrypt(and_gate, cks)
expected = False
print(f"{actual=}, {expected=}")
