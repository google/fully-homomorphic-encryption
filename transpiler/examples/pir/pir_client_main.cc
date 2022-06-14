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

#include <stdlib.h>

#include <ios>
#include <iostream>
#include <string>

#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "transpiler/data/tfhe_data.h"
#include "transpiler/examples/pir/pir_api.h"
#include "transpiler/examples/pir/pir_cloud_service.h"
#include "xls/common/logging/logging.h"

constexpr int kMainMinimumLambda = 120;

// Random seed for key generation
// Note: In real applications, a cryptographically secure seed needs to be used.
constexpr std::array<uint32_t, 3> kSeed = {314, 1592, 657};

using ::fully_homomorphic_encryption::CloudService;
using ::fully_homomorphic_encryption::Index;
using ::fully_homomorphic_encryption::RecordT;

using TfheIndex = Tfhe<unsigned char>;
using TfheRecordT = Tfhe<unsigned char>;

int main() {
  TFHEParameters params(kMainMinimumLambda);
  TFHESecretKeySet key(params, kSeed);

  // Construct "database"
  std::vector<RecordT> database_plaintext;
  while (database_plaintext.size() < kDbSize) {
    std::cout << absl::StrFormat(
        "Please enter a single character for storage (%d more; ENTER to "
        "quit): ",
        kDbSize - database_plaintext.size());
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) {
      std::cerr << "No input provided." << std::endl;
      return 1;  // Exit program
    }

    if (line.size() == 1) {
      database_plaintext.push_back(line[0]);
    } else {
      std::cerr << "Invalid argument: '" << line << "'." << std::endl;
    }
  }

  // Pass encrypted "database" to CloudService, which retains ownership
  std::cout << "Establishing connection." << std::endl;
  auto database_ciphertext =
      TfheArray<RecordT, kDbSize>::Encrypt(database_plaintext, key);
  CloudService service(std::move(database_ciphertext));

  while (true) {
    std::cout << absl::StrFormat(
        "Please enter a valid index [0, %d) (ENTER to quit): ", kDbSize);
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) {
      std::cout << "Closing connection." << std::endl;
      break;
    }

    // kDbSize should be a value in [0, 255] as specified by the service, so we
    // can safely `static_cast` to `Index`.
    int index = 0;
    if (!absl::SimpleAtoi(line, &index) || !(index >= 0 && index < kDbSize)) {
      std::cerr << "Invalid argument: '" << line << "'." << std::endl;
      continue;
    }

    // Encrypt and query
    std::cout << "Querying the database..." << std::endl;
    auto index_ciphertext = TfheIndex::Encrypt(static_cast<Index>(index), key);
    TfheRecordT result_ciphertext(params);
    XLS_CHECK_OK(
        service.QueryRecord(result_ciphertext, index_ciphertext, key.cloud()));

    const RecordT result = result_ciphertext.Decrypt(key);
    std::cout << "Result: '" << result << "'." << std::endl;
  }

  return 0;
}
