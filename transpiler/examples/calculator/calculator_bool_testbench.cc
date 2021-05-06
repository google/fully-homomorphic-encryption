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

#include "absl/container/fixed_array.h"
#include "transpiler/data/boolean_data.h"
#include "transpiler/examples/calculator/calculator_bool.h"
#include "xls/common/logging/logging.h"

using namespace std;

const int int_size = 32;

void calculate(short x, short y, char op) {
  cout << "inputs are " << x << " " << op << " " << y << endl;
  // Encode data
  EncodedShort ciphertext_x(x);
  EncodedShort ciphertext_y(y);
  EncodedChar ciphertext_op(op);
  cout << "Encoding done" << endl;

  cout << "Initial state check: " << endl;
  cout << ciphertext_x.Decode();
  cout << "  ";
  cout << ciphertext_y.Decode();
  cout << "  ";
  cout << ciphertext_op.Decode();
  cout << "\n";

  cout << "\t\t\t\t\tServer side computation:" << endl;
  // Perform calculation
  EncodedShort cipher_result(int_size);
  absl::FixedArray<bool> calculator = {true};

  XLS_CHECK_OK(my_package(cipher_result.get(), absl::MakeSpan(calculator),
                          ciphertext_x.get(), ciphertext_y.get(),
                          ciphertext_op.get()));

  cout << "\t\t\t\t\tComputation done" << endl;

  cout << "Decoded result: ";
  // Decode results.

  cout << cipher_result.Decode();

  cout << "\n";
  cout << "Decoding done" << endl << endl;
}

int main(int argc, char** argv) {
  calculate(10, 20, '*');
  calculate(10, 20, '+');
  calculate(10, 20, '-');
}
