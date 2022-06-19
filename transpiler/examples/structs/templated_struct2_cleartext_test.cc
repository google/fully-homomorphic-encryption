#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "templated_struct2.h"
#include "transpiler/data/cleartext_data.h"
#include "transpiler/examples/structs/templated_struct2.h"
#ifdef USE_YOSYS_INTERPRETED_CLEARTEXT
#include "transpiler/examples/structs/templated_struct2_yosys_interpreted_cleartext.h"
#include "transpiler/examples/structs/templated_struct2_yosys_interpreted_cleartext.types.h"
#else
#include "transpiler/examples/structs/templated_struct2_cleartext.h"
#include "transpiler/examples/structs/templated_struct2_cleartext.types.h"
#endif

#include "xls/common/status/matchers.h"

TEST(TemplatedStruct2Test, TemplatedStruct2CleartextTest) {
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

  Encoded<Array<Tag<int>, LEN>> encoded_in(in);
  Encoded<Tag<Array<short, (LEN << 1)>>> encoded_out;
  XLS_CHECK_OK(convert(encoded_in, encoded_out));
  Tag<Array<short, (LEN << 1)>> decoded_out = encoded_out.Decode();

  ASSERT_EQ(0, memcmp(&decoded_out, &reference_out, sizeof(decoded_out)));
}
