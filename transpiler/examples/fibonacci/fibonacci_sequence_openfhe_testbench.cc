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
#include "openfhe/binfhe/binfhecontext.h"
#include "transpiler/data/openfhe_data.h"
#include "transpiler/examples/fibonacci/fibonacci_sequence.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_OPENFHE
#include "transpiler/examples/fibonacci/fibonacci_sequence_interpreted_openfhe.h"
#else
#include "transpiler/examples/fibonacci/fibonacci_sequence_openfhe.h"
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
  int input = 7;
  auto encrypted_input = OpenFhe<int>::Encrypt(input, cc, sk);
  std::cout << absl::StreamFormat("Decrypted input: %d",
                                  encrypted_input.Decrypt(sk))
            << std::endl;

  OpenFheArray<int, FIBONACCI_SEQUENCE_SIZE> encrypted_result(cc);
  XLS_CHECK_OK(fibonacci_sequence(encrypted_input, encrypted_result, cc));

  absl::FixedArray<int> result = encrypted_result.Decrypt(sk);
  for (int i = 0; i < result.size(); i++) {
    std::cout << absl::StrFormat("Result %d: %d", i, result[i]) << std::endl;
  }

  return 0;
}
