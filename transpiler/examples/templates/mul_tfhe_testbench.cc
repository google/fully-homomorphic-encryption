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

// Simplistic testbench demonstrating the benefit of templatizing code to use
// the minimum possible number of bits.

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "transpiler/data/tfhe_data.h"
#ifdef USE_INTERPRETED_TFHE
#include "transpiler/examples/templates/mul16_interpreted_tfhe.h"
#include "transpiler/examples/templates/mul8_interpreted_tfhe.h"
#else
#include "transpiler/examples/templates/mul16_tfhe.h"
#include "transpiler/examples/templates/mul8_tfhe.h"
#endif

namespace fhe {

absl::Status RealMain() {
  constexpr int kMinimumLambda = 120;
  // Generate a random key.
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  TFHEParameters params(kMinimumLambda);
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  int a = rand();
  int b = rand();
  int c = rand();
  auto x_a = Tfhe<int>::Encrypt(a, key);
  auto x_b = Tfhe<int>::Encrypt(b, key);
  auto x_c = Tfhe<int>::Encrypt(c, key);
  auto x_result = Tfhe<int>(params);

  Tfhe<short> short_result(params);
  absl::Time start_time = absl::Now();
  absl::Status status = Mul16(short_result, Tfhe<short>::Encrypt(a, key),
                              Tfhe<short>::Encrypt(b, key),
                              Tfhe<short>::Encrypt(c, key), key.cloud());
  if (!status.ok()) {
    return status;
  }
  std::cout << "Time to multiply/add, 16-bit version : "
            << absl::Now() - start_time << std::endl;

  Tfhe<unsigned char> char_result(params);
  start_time = absl::Now();
  status = Mul8(char_result, Tfhe<unsigned char>::Encrypt(a, key),
                Tfhe<unsigned char>::Encrypt(b, key),
                Tfhe<unsigned char>::Encrypt(c, key), key.cloud());
  if (!status.ok()) {
    return status;
  }
  std::cout << "Time to multiply/add, 8-bit version : "
            << absl::Now() - start_time << std::endl;

  return absl::OkStatus();
}

}  // namespace fhe

int main(int argc, char** argv) {
  absl::Status status = fhe::RealMain();
  return !status.ok();
}
