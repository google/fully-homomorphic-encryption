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

#include "palisade/binfhe/binfhecontext.h"
#include "testing/base/public/benchmark.h"
#include "testing/base/public/gmock.h"
#include "testing/base/public/gunit.h"
#include "transpiler/data/palisade_data.h"

#ifdef USE_YOSYS_OPTIMIZER
#include "transpiler/benchmarks/add_char_yosys_interpreted_palisade.h"
#include "transpiler/benchmarks/add_int_yosys_interpreted_palisade.h"
#else
#include "transpiler/benchmarks/add_char_xls_interpreted_palisade.h"
#include "transpiler/benchmarks/add_int_xls_interpreted_palisade.h"
#endif

// PALISADE parameters
constexpr lbcrypto::BINFHEPARAMSET kSecurityLevel = lbcrypto::MEDIUM;

void BM_AddChar(benchmark::State& state) {
  auto cc = lbcrypto::BinFHEContext();
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  auto a = PalisadeChar::Encrypt('a', cc, sk);
  auto b = PalisadeChar::Encrypt('b', cc, sk);
  PalisadeChar result(cc);

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

  auto a = PalisadeInt::Encrypt('a', cc, sk);
  auto b = PalisadeInt::Encrypt('b', cc, sk);
  PalisadeInt result(cc);

  for (auto s : state) {
    ASSERT_OK(AddInt(result, a, b, cc));
  }
}
BENCHMARK(BM_AddInt);
