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

#include <iostream>

#include "transpiler/data/cleartext_data.h"
#include "transpiler/examples/simple_sum/simple_sum_cleartext.h"
#include "xls/common/logging/logging.h"

using namespace std;

int main(int argc, char** argv) {
  // create inputs.
  int x = 4052;
  int y = 913;

  cout << "inputs are " << x << " and " << y << ", sum: " << x + y << endl;
  // Encode data
  EncodedInt ciphertext_x(x);
  EncodedInt ciphertext_y(y);
  cout << "Encoding done" << endl;

  cout << "Initial state check: " << endl;
  cout << ciphertext_x.Decode();
  cout << "  ";
  cout << ciphertext_y.Decode();
  cout << "\n";

  cout << "\t\t\t\t\tServer side computation:" << endl;
  // Perform addition
  EncodedInt cipher_result;
  XLS_CHECK_OK(simple_sum(cipher_result, ciphertext_x, ciphertext_y));

  cout << "\t\t\t\t\tComputation done" << endl;

  cout << "Decoded result: ";
  cout << cipher_result.Decode();

  cout << "\n";
  cout << "Decoding done" << endl;
}
