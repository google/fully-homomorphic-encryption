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

#include "sum3d.h"

#pragma hls_top
int sum3d(unsigned char a[X_DIM][Y_DIM][Z_DIM],
          unsigned char b[X_DIM][Y_DIM][Z_DIM]) {
  int sum = 0;
#pragma hls_unroll yes
  for (int x = 0; x < X_DIM; x++) {
#pragma hls_unroll yes
    for (int y = 0; y < Y_DIM; y++) {
#pragma hls_unroll yes
      for (int z = 0; z < Z_DIM; z++) {
        sum += a[x][y][z] + b[x][y][z];
      }
    }
  }
  return sum;
}
