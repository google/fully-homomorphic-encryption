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

#include "openfhe/binfhe/binfhecontext.h"
#include "transpiler/data/openfhe_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_OPENFHE
#include "transpiler/examples/calculator/calculator_interpreted_openfhe.h"
#else
#include "transpiler/examples/calculator/calculator_openfhe.h"
#endif

using namespace std;

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

void calculate(short x, short y, char op, lbcrypto::BinFHEContext cc,
               lbcrypto::LWEPrivateKey sk) {
  cout << "inputs are " << x << " " << op << " " << y << endl;
  // Encrypt data
  auto encryptedX = OpenFhe<short>::Encrypt(x, cc, sk);
  auto encryptedY = OpenFhe<short>::Encrypt(y, cc, sk);
  auto encryptedOp = OpenFhe<char>::Encrypt(op, cc, sk);

  cout << "Encryption done" << endl;
  cout << "Initial state check by decryption: " << endl;
  // Decrypt results.
  cout << encryptedX.Decrypt(sk);
  cout << "  ";
  cout << encryptedY.Decrypt(sk);
  cout << "  ";
  cout << encryptedOp.Decrypt(sk);
  cout << "  ";

  cout << "\n";
  cout << "\t\t\t\t\tServer side computation:" << endl;
  // Perform computation
  OpenFhe<short> encryptedResult(cc);
  OpenFhe<Calculator> calc(cc);
  calc.SetUnencrypted(Calculator(), &cc);

  absl::Time start_time = absl::Now();
  double cpu_start_time = clock();
  XLS_CHECK_OK(my_package(encryptedResult, calc, encryptedX, encryptedY,
                          encryptedOp, cc));
  double cpu_end_time = clock();
  absl::Time end_time = absl::Now();
  cout << "\t\t\t\t\tComputation done" << endl;
  cout << "\t\t\t\t\tTotal time: "
       << absl::ToDoubleSeconds(end_time - start_time) << " secs" << endl;
  cout << "\t\t\t\t\t  CPU time: "
       << (cpu_end_time - cpu_start_time) / 1'000'000 << " secs" << endl;

  // Decrypt results.
  cout << "Decrypted result: ";
  cout << encryptedResult.Decrypt(sk);
  cout << "\n";
  cout << "Decryption done" << endl << endl;
}

int main(int argc, char** argv) {
  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  calculate(10, 20, '*', cc, sk);
  calculate(10, 20, '+', cc, sk);
  calculate(10, 20, '-', cc, sk);
}
