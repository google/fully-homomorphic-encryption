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

#include "openfhe/binfhe/binfhecontext.h"
#include "transpiler/examples/structs/simple_struct_openfhe.h"
#include "transpiler/examples/structs/simple_struct_openfhe.types.h"
#include "xls/common/logging/logging.h"

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

  SimpleStruct simple_struct;
  simple_struct.a = 2;
  simple_struct.b = -4;
  simple_struct.c = 8;

  OpenFhe<SimpleStruct> fhe_simple_struct = {cc};
  fhe_simple_struct.SetEncrypted(simple_struct, sk);

  std::cout << "Initial round-trip check: " << std::endl;
  SimpleStruct round_trip = fhe_simple_struct.Decrypt(sk);
  std::cout << "  A: " << static_cast<int>(round_trip.a) << std::endl;
  std::cout << "  B: " << static_cast<int>(round_trip.b) << std::endl;
  std::cout << "  C: " << static_cast<int>(round_trip.c) << std::endl
            << std::endl;

  std::cout << "Starting computation." << std::endl;
  OpenFhe<int> fhe_result = {cc};
  XLS_CHECK_OK(SumSimpleStruct(fhe_result, fhe_simple_struct, cc));

  int result = fhe_result.Decrypt(sk);
  std::cout << "Done. Result: " << result << std::endl;
  return 0;
}
