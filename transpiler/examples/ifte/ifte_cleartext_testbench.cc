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
#include <stdint.h>

#include <iostream>

#include "transpiler/data/cleartext_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_YOSYS_INTERPRETED_CLEARTEXT
#include "transpiler/examples/ifte/ifte_yosys_interpreted_cleartext.h"
#else
#include "transpiler/examples/ifte/ifte_cleartext.h"
#endif

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "usage: ifte i t e" << std::endl;
    return 1;
  }

  char i = argv[1][0];
  if (i == '.') i = 0;
  char t = argv[2][0];
  char e = argv[3][0];

  std::cout << "i: " << i << std::endl;
  std::cout << "t: " << t << std::endl;
  std::cout << "e: " << e << std::endl;

  // Encrypt data
  Encoded<bool> encoded_i;
  encoded_i.Encode(!!i);
  Encoded<char> encoded_t;
  encoded_t.Encode(t);
  Encoded<char> encoded_e;
  encoded_e.Encode(e);

  std::cout << "Encryption done" << std::endl;

  std::cout << "Initial state check by decryption: " << std::endl;
  // Decrypt results.
  std::cout << static_cast<char>(encoded_i.Decode());
  std::cout << "  ";
  std::cout << static_cast<char>(encoded_t.Decode());
  std::cout << "  ";
  std::cout << static_cast<char>(encoded_e.Decode());

  std::cout << "\n";

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Perform addition
  Encoded<char> cipher_result;
  XLS_CHECK_OK(ifte(cipher_result, encoded_i, encoded_t, encoded_e));

  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  std::cout << "Decrypted result: " << (char)cipher_result.Decode() << "\n";
  std::cout << "Decryption done" << std::endl;
}
