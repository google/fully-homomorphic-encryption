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
#include "openfhe/binfhe/binfhecontext.h"
#include "transpiler/data/openfhe_data.h"

#ifdef USE_YOSYS_OPTIMIZER
#include "transpiler/benchmarks/add_char_yosys_openfhe.h"
#include "transpiler/benchmarks/add_int_yosys_openfhe.h"
#else
#include "transpiler/benchmarks/add_char_xls_openfhe.h"
#include "transpiler/benchmarks/add_int_xls_openfhe.h"
#endif

// OpenFHE parameters
constexpr lbcrypto::BINFHE_PARAMSET kSecurityLevel = lbcrypto::MEDIUM;

void BM_AddChar(benchmark::State& state) {
  auto cc = lbcrypto::BinFHEContext();
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  auto a = OpenFhe<char>::Encrypt('a', cc, sk);
  auto b = OpenFhe<char>::Encrypt('b', cc, sk);
  OpenFhe<char> result(cc);

  for (auto s : state) {
    benchmark::DoNotOptimize(AddChar(result, a, b, cc));
  }
}
BENCHMARK(BM_AddChar);

void BM_AddInt(benchmark::State& state) {
  auto cc = lbcrypto::BinFHEContext();
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  auto a = OpenFhe<int>::Encrypt('a', cc, sk);
  auto b = OpenFhe<int>::Encrypt('b', cc, sk);
  OpenFhe<int> result(cc);

  for (auto s : state) {
    benchmark::DoNotOptimize(AddInt(result, a, b, cc));
  }
}
BENCHMARK(BM_AddInt);
