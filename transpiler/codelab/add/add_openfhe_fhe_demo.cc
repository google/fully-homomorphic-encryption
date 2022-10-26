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

#include <iostream>
#include <ostream>

#include "absl/strings/numbers.h"
#include "transpiler/codelab/add/add_fhe_lib.h"
#include "transpiler/data/openfhe_data.h"

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: add_main [int] [int]\n\n");
    return 1;
  }

  int x, y;
  XLS_CHECK(absl::SimpleAtoi(argv[1], &x));
  XLS_CHECK(absl::SimpleAtoi(argv[2], &y));
  std::cout << "Computing " << x << " + " << y << std::endl;

  // Set up backend context and encryption keys.
  auto context = lbcrypto::BinFHEContext();
  context.GenerateBinFHEContext(kSecurityLevel);
  auto sk = context.KeyGen();
  context.BTKeyGen(sk);

  auto ciphertext_x = OpenFhe<signed int>::Encrypt(x, context, sk);
  auto ciphertext_y = OpenFhe<signed int>::Encrypt(y, context, sk);
  OpenFhe<signed int> result(context);
  XLS_CHECK_OK(add(result, ciphertext_x, ciphertext_y, context));

  std::cout << "Result: " << result.Decrypt(sk) << "\n";
  return 0;
}
