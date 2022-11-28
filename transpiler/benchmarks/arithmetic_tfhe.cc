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

#include <stdint.h>

#include "benchmark/benchmark.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "transpiler/data/tfhe_data.h"

#ifdef USE_YOSYS_OPTIMIZER
#include "transpiler/benchmarks/add_char_yosys_tfhe.h"
#include "transpiler/benchmarks/add_int_yosys_tfhe.h"
#else
#include "transpiler/benchmarks/add_char_xls_tfhe.h"
#include "transpiler/benchmarks/add_int_xls_tfhe.h"
#endif

// TFHE parameters
// Note: in real applications, a cryptographically-secure seed must be used.
constexpr int kMainMinimumLambda = 120;
constexpr std::array<uint32_t, 3> kSeed = {314, 1592, 657};

void BM_AddChar(benchmark::State& state) {
  TFHEParameters params(kMainMinimumLambda);
  TFHESecretKeySet key(params, kSeed);

  auto a = Tfhe<char>::Encrypt('a', key);
  auto b = Tfhe<char>::Encrypt('b', key);
  Tfhe<char> result(key.params());

  for (auto s : state) {
    benchmark::DoNotOptimize(AddChar(result, a, b, key.cloud()));
  }
}
BENCHMARK(BM_AddChar);

void BM_AddInt(benchmark::State& state) {
  TFHEParameters params(kMainMinimumLambda);
  TFHESecretKeySet key(params, kSeed);

  auto a = Tfhe<int>::Encrypt('a', key);
  auto b = Tfhe<int>::Encrypt('b', key);
  Tfhe<int> result(key.params());

  for (auto s : state) {
    ASSERT_OK(AddInt(result, a, b, key.cloud()));
  }
}
BENCHMARK(BM_AddInt);
