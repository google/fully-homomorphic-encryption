#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "templated_struct2.h"
#include "transpiler/data/tfhe_data.h"
#include "transpiler/examples/structs/templated_struct2.h"
#ifdef USE_YOSYS_INTERPRETED_TFHE
#include "transpiler/examples/structs/templated_struct2_yosys_interpreted_tfhe.h"
#include "transpiler/examples/structs/templated_struct2_yosys_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/structs/templated_struct2_interpreted_tfhe.h"
#include "transpiler/examples/structs/templated_struct2_interpreted_tfhe.types.h"
#endif

#include "xls/common/status/matchers.h"

TEST(TemplatedStruct2Test, TemplatedStruct2TfheTest) {
  constexpr int kMainMinimumLambda = 120;
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  Array<Tag<int>, LEN> in = {{
      {0x12345678},
      {0x23456789},
      {0x3456789a},
      {0x3456789a},
      {0x456789ab},
      {0x56789abc},
      {0x6789abcd},
      {0x789abcde},
  }};

  Tag<Array<short, (LEN << 1)>> reference_out;
  convert(in, reference_out);

  Tfhe<Array<Tag<int>, LEN>> tfhe_in(params);
  tfhe_in.SetEncrypted(in, key);
  Tfhe<Tag<Array<short, (LEN << 1)>>> tfhe_out(params);
  XLS_CHECK_OK(convert(tfhe_in, tfhe_out, key.cloud()));
  Tag<Array<short, (LEN << 1)>> decoded_out = tfhe_out.Decrypt(key);

  ASSERT_EQ(0, memcmp(&decoded_out, &reference_out, sizeof(decoded_out)));
}
