#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "templated_struct2.h"
#include "transpiler/data/openfhe_data.h"
#include "transpiler/examples/structs/templated_struct2.h"
#ifdef USE_YOSYS_INTERPRETED_OPENFHE
#include "transpiler/examples/structs/templated_struct2_yosys_interpreted_openfhe.h"
#include "transpiler/examples/structs/templated_struct2_yosys_interpreted_openfhe.types.h"
#else
#include "transpiler/examples/structs/templated_struct2_interpreted_openfhe.h"
#include "transpiler/examples/structs/templated_struct2_interpreted_openfhe.types.h"
#endif

#include "xls/common/status/matchers.h"

TEST(TemplatedStruct2Test, TemplatedStruct2OpenFheTest) {
  // Generate a keyset.
  auto cc = lbcrypto::BinFHEContext();
  cc.GenerateBinFHEContext(lbcrypto::MEDIUM);

  // Generate a "random" key.
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::cout << "Generating the secret key..." << std::endl;
  auto sk = cc.KeyGen();

  std::cout << "Generating the bootstrapping keys..." << std::endl;
  cc.BTKeyGen(sk);

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

  OpenFhe<Array<Tag<int>, LEN>> openfhe_in(cc);
  openfhe_in.SetEncrypted(in, sk);
  OpenFhe<Tag<Array<short, (LEN << 1)>>> openfhe_out(cc);
  XLS_CHECK_OK(convert(openfhe_in, openfhe_out, cc));
  Tag<Array<short, (LEN << 1)>> decoded_out = openfhe_out.Decrypt(sk);

  ASSERT_EQ(0, memcmp(&decoded_out, &reference_out, sizeof(decoded_out)));
}
