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

#include <stdio.h>
#include <time.h>

#include <array>
#include <iostream>
#include <string>
#include <vector>

#include "absl/strings/str_format.h"
#include "palisade/binfhe/binfhecontext.h"
#include "transpiler/data/palisade_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_PALISADE
#include "transpiler/examples/fibonacci/fibonacci_sequence_interpreted_palisade.h"
#else
#include "transpiler/examples/fibonacci/fibonacci_sequence_palisade.h"
#endif

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

int main(int argc, char** argv) {
  // Generate a keyset.
  auto cc = lbcrypto::BinFHEContext();

  // Generate a random key.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  // Create inputs.
  int input = 5;
  auto encrypted_input = PalisadeValue<int>::Encrypt(input, cc, sk);
  std::cout << absl::StreamFormat("Decrypted input: %d",
                                  encrypted_input.Decrypt(sk))
            << std::endl;

  PalisadeArray<int> encrypted_result(500, cc);
  XLS_CHECK_OK(
      fibonacci_sequence(encrypted_input.get(), encrypted_result.get(), cc));
  absl::FixedArray<int> result = encrypted_result.Decrypt(sk);
  std::cout << absl::StrFormat("Result: %d, %d, %d, %d, %d", result[0],
                               result[1], result[2], result[3], result[4])
            << std::endl;

  return 0;
}
