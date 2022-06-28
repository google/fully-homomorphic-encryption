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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_SUM3D_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_SUM3D_H_

// Demonstrates multidimensional array support:
// Adds all numbers in the specified arrays.
#define X_DIM 2
#define Y_DIM 3
#define Z_DIM 2
int sum3d(unsigned char a[X_DIM][Y_DIM][Z_DIM],
          unsigned char b[X_DIM][Y_DIM][Z_DIM]);

void inc3d(unsigned char a[X_DIM][Y_DIM][Z_DIM]);

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_SUM3D_H_
