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

#include <cstdio>
#include <iostream>

#include "fibonacci.h"  // NOLINT: xlscc currently needs locally-scoped includes.
#include "fibonacci_sequence.h"  // NOLINT: ditto.

using namespace std;

void call_fibonacci_sequence(int n) {
  cout << "5 numbers of Fibonacci sequence starting from " << n << endl;
  int result[5];
  fibonacci_sequence(n, result);
  for (int i = 0; i < 5; i++) {
    cout << result[i] << endl;
  }
  cout << endl;
}

void test_fibonacci_sequence() {
  call_fibonacci_sequence(0);
  call_fibonacci_sequence(1);
  call_fibonacci_sequence(2);
  call_fibonacci_sequence(5);
  call_fibonacci_sequence(10);
  // Values outside [0, 10] aren't supported. Hence, some garbage should be
  // printed below.
  call_fibonacci_sequence(-1);
  call_fibonacci_sequence(20);
}

void test_fibonacci_number() {
  cout << "Fibonacci(-1) : " << fibonacci_number(-1) << endl;
  cout << "Fibonacci(0) : " << fibonacci_number(0) << endl;
  cout << "Fibonacci(1) : " << fibonacci_number(1) << endl;
  cout << "Fibonacci(2) : " << fibonacci_number(2) << endl;
  cout << "Fibonacci(3) : " << fibonacci_number(3) << endl;
  cout << "Fibonacci(4) : " << fibonacci_number(4) << endl;
  cout << "Fibonacci(10) : " << fibonacci_number(10) << endl;
  cout << "Fibonacci(30) : " << fibonacci_number(30) << endl;
  // Should return -1 because fibonacci() supports only input between 0 and 30.
  cout << "Fibonacci(40) : " << fibonacci_number(40) << endl;
}

int main(int argc, char** argv) {
  cout << "Testing Fibonacci sequence" << endl;
  test_fibonacci_sequence();
  cout << "Testing Fibonacci number" << endl;
  test_fibonacci_number();
  return 0;
}
