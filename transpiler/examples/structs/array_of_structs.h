// Copyright 2022 Google LLC
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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_ARRAY_OF_STRUCTS_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_ARRAY_OF_STRUCTS_H_

// Example demonstrating successful manipulation of arrays of arrays...of
// structs.
// The index sizes are low to keep compilation times from exploding.

#define DIM_X 2
#define DIM_Y 2
#define DIM_Z 2

struct Simple {
  unsigned int v;
};

// Doubles the sum of the elements in the input array.
Simple DoubleSimpleArray(const Simple data[DIM_X]);
// Doubles the sum of the elements in the input 3D array.
Simple DoubleSimpleArray3D(const Simple data[DIM_X][DIM_Y][DIM_Z]);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_STRUCTS_ARRAY_OF_STRUCTS_H_
