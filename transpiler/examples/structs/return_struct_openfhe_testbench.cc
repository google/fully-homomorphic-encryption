// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdint>
#include <iostream>

#include "xls/common/logging/logging.h"
#ifdef USE_INTERPRETED_OPENFHE
#include "transpiler/examples/structs/return_struct_interpreted_openfhe.h"
#include "transpiler/examples/structs/return_struct_interpreted_openfhe.types.h"
#else
#include "transpiler/examples/structs/return_struct_openfhe.h"
#include "transpiler/examples/structs/return_struct_openfhe.types.h"
#endif
#include "openfhe/binfhe/binfhecontext.h"

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

int main(int argc, char** argv) {
  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  Embedded embedded;
  embedded.a = -1;
  embedded.b = 2;
  embedded.c = -4;

  OpenFhe<Embedded> fhe_embedded(cc);
  fhe_embedded.SetEncrypted(embedded, sk);

  std::cout << "Initial round-trip check: " << std::endl;
  Embedded round_trip = fhe_embedded.Decrypt(sk);
  std::cout << "  A: " << static_cast<int>(round_trip.a) << std::endl;
  std::cout << "  B: " << static_cast<int>(round_trip.b) << std::endl;
  std::cout << "  C: " << static_cast<int>(round_trip.c) << std::endl;

  std::cout << "Starting computation." << std::endl;
  OpenFhe<ReturnStruct> fhe_result(cc);
  auto fhe_a = OpenFhe<char>::Encrypt(8, cc, sk);
  auto fhe_c = OpenFhe<char>::Encrypt(16, cc, sk);
  XLS_CHECK_OK(
      ConstructReturnStruct(fhe_result, fhe_a, fhe_embedded, fhe_c, cc));

  ReturnStruct result = fhe_result.Decrypt(sk);
  std::cout << "Done. Result: " << std::endl;
  std::cout << " - a: " << static_cast<int>(result.a) << std::endl;
  std::cout << " - b.a: " << static_cast<int>(result.b.a) << std::endl;
  std::cout << " - b.b: " << static_cast<int>(result.b.b) << std::endl;
  std::cout << " - b.c: " << static_cast<int>(result.b.c) << std::endl;
  std::cout << " - c: " << static_cast<int>(result.c) << std::endl;
  return 0;
}
