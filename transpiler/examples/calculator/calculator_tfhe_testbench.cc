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
#include "transpiler/examples/calculator/calculator_interpreted_tfhe.h"
#include "transpiler/examples/calculator/calculator_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/calculator/calculator_tfhe.h"
#include "transpiler/examples/calculator/calculator_tfhe.types.h"
#endif

using namespace std;

const int kMainMinimumLambda = 120;

void calculate(short x, short y, char op, TFHEParameters& params,
               TFHESecretKeySet& key) {
  cout << "inputs are " << x << " " << op << " " << y << endl;
  // Encrypt data
  auto encryptedX = Tfhe<short>::Encrypt(x, key);
  auto encryptedY = Tfhe<short>::Encrypt(y, key);
  auto encryptedOp = Tfhe<char>::Encrypt(op, key);

  cout << "Encryption done" << endl;
  cout << "Initial state check by decryption: " << endl;
  // Decrypt results.
  cout << encryptedX.Decrypt(key);
  cout << "  ";
  cout << encryptedY.Decrypt(key);
  cout << "  ";
  cout << encryptedOp.Decrypt(key);
  cout << "  ";

  cout << "\n";
  cout << "\t\t\t\t\tServer side computation:" << endl;
  // Perform computation
  Tfhe<short> encryptedResult(params);
  Tfhe<Calculator> calc(params);
  calc.SetUnencrypted(Calculator(), key.cloud());

  absl::Time start_time = absl::Now();
  double cpu_start_time = clock();
  XLS_CHECK_OK(my_package(encryptedResult, calc, encryptedX, encryptedY,
                          encryptedOp, key.cloud()));
  double cpu_end_time = clock();
  absl::Time end_time = absl::Now();
  cout << "\t\t\t\t\tComputation done" << endl;
  cout << "\t\t\t\t\tTotal time: "
       << absl::ToDoubleSeconds(end_time - start_time) << " secs" << endl;
  cout << "\t\t\t\t\t  CPU time: "
       << (cpu_end_time - cpu_start_time) / 1'000'000 << " secs" << endl;

  // Decrypt results.
  cout << "Decrypted result: ";
  cout << encryptedResult.Decrypt(key);
  cout << "\n";
  cout << "Decryption done" << endl << endl;
}

int main(int argc, char** argv) {
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  calculate(10, 20, '*', params, key);
  calculate(10, 20, '+', params, key);
  calculate(10, 20, '-', params, key);
}
