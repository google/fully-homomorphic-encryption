"""Tests for add_one."""

from absl.testing import absltest
from absl.testing import parameterized
from transpiler.examples.add_one import add_one_fhe_py_lib
from transpiler.examples.add_one import add_one_fhe_py_lib_lut2
from transpiler.examples.add_one import add_one_fhe_py_lib_lut3
from transpiler.examples.add_one import add_one_lib


class AddOneTest(parameterized.TestCase):

  @parameterized.named_parameters(
      dict(testcase_name='no_luts', fhe_lib=add_one_fhe_py_lib),
      dict(testcase_name='lut2', fhe_lib=add_one_fhe_py_lib_lut2),
      dict(testcase_name='lut3', fhe_lib=add_one_fhe_py_lib_lut3),
  )
  def test_add_one(self, fhe_lib):
    x = 2
    boolean_params, cks, sks, ciphertext_x = add_one_lib.setup(x)

    result_ciphertext = fhe_lib.add_one(
        ciphertext_x,
        sks,
        boolean_params,
    )

    result = add_one_lib.decrypt(result_ciphertext, cks)
    self.assertEqual(x + 1, result)


if __name__ == '__main__':
  absltest.main()
