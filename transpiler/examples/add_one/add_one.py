"""A demonstration of adding 1 to a number in FHE."""
from collections.abc import Sequence

from absl import app

from transpiler.examples.add_one import add_one_fhe_py_lib
from transpiler.examples.add_one import add_one_lib


def main(argv: Sequence[str]) -> None:
  if len(argv) != 2:
    raise app.UsageError('Expected usage: add_one <x>')

  _, x = argv
  x = int(x)

  boolean_params, cks, sks, ciphertext_x = add_one_lib.setup(x)

  print('Running FHE circuit')
  result_ciphertext = add_one_fhe_py_lib.add_one(
      ciphertext_x,
      sks,
      boolean_params,
  )

  result = add_one_lib.decrypt(result_ciphertext, cks)
  print(f'Result: {result}')


if __name__ == '__main__':
  app.run(main)
