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

#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
#include "transpiler/data/tfhe_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_TFHE
#include "transpiler/examples/fibonacci/fibonacci_interpreted_tfhe.h"
#else
#include "transpiler/examples/fibonacci/fibonacci_tfhe.h"
#endif

using namespace std;

const int kMainMinimumLambda = 120;

void test_fibonacci_number() {
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  // create inputs.
  int inputs[6];
  inputs[0] = -1;
  inputs[1] = 0;
  inputs[2] = 1;
  inputs[3] = 3;
  inputs[4] = 10;

  for (int i = 0; i < 5; i++) {
    int n = inputs[i];
    cout << "input n = " << n << endl;

    // Encrypt the input value
    auto encryptedN = Tfhe<int>::Encrypt(n, key);
    cout << "Encryption done" << endl;
    cout << "Initial state check by decryption: " << endl;
    cout << "Decrypted input: " << encryptedN.Decrypt(key) << endl;

    // Perform computation
    cout << "\t\t\t\t\tServer side computation:" << endl;
    Tfhe<int> encryptedResult(params);

    absl::Time start_time = absl::Now();
    double cpu_start_time = clock();
    XLS_CHECK_OK(fibonacci_number(encryptedResult, encryptedN, key.cloud()));
    double cpu_end_time = clock();
    absl::Time end_time = absl::Now();
    cout << "\t\t\t\t\tComputation done" << endl;
    cout << "\t\t\t\t\tTotal time: "
         << absl::ToDoubleSeconds(end_time - start_time) << " secs" << endl;
    cout << "\t\t\t\t\t  CPU time: "
         << (cpu_end_time - cpu_start_time) / 1'000'000 << " secs" << endl;

    cout << "Decrypted result: ";
    // Decrypt results.
    cout << encryptedResult.Decrypt(key) << endl;
    cout << "Decryption done" << endl << endl;
  }
}

int main(int argc, char** argv) { test_fibonacci_number(); }
