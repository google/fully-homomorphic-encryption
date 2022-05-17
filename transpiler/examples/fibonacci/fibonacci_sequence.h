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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_FIBONACCI_FIBONACCI_SEQUENCE_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_FIBONACCI_FIBONACCI_SEQUENCE_H_

#define FIBONACCI_SEQUENCE_SIZE 5

// Computes 5 values in Fibonacci sequence starting from n'th number.
// n must be between 0 and 10.
void fibonacci_sequence(int n, int output[FIBONACCI_SEQUENCE_SIZE]);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_FIBONACCI_FIBONACCI_SEQUENCE_H_
