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

#include "transpiler/palisade_runner.h"

#include <vector>

#include "absl/status/statusor.h"
#include "absl/types/optional.h"
#include "palisade/binfhe/binfhecontext.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

lbcrypto::LWECiphertext PalisadeRunner::PalisadeOperations::And(
    const lbcrypto::LWECiphertext lhs, const lbcrypto::LWECiphertext rhs) {
  if (lhs == rhs) {
    return lhs;
  }
  return cc_.EvalBinGate(lbcrypto::AND, lhs, rhs);
}
lbcrypto::LWECiphertext PalisadeRunner::PalisadeOperations::Or(
    const lbcrypto::LWECiphertext lhs, const lbcrypto::LWECiphertext rhs) {
  if (lhs == rhs) {
    return lhs;
  }
  return cc_.EvalBinGate(lbcrypto::OR, lhs, rhs);
}
lbcrypto::LWECiphertext PalisadeRunner::PalisadeOperations::Not(
    const lbcrypto::LWECiphertext in) {
  return cc_.EvalNOT(in);
}

lbcrypto::LWECiphertext PalisadeRunner::PalisadeOperations::Constant(
    bool value) {
  return cc_.EvalConstant(value);
}

void PalisadeRunner::PalisadeOperations::Copy(const lbcrypto::LWECiphertext src,
                                              lbcrypto::LWECiphertext& dst) {
  dst = src;
}

lbcrypto::LWECiphertext PalisadeRunner::PalisadeOperations::CopyOf(
    const lbcrypto::LWECiphertext src) {
  return src;
}

absl::Status PalisadeRunner::Run(
    absl::Span<lbcrypto::LWECiphertext> result,
    absl::flat_hash_map<std::string, absl::Span<lbcrypto::LWECiphertext>> args,
    lbcrypto::BinFHEContext cc) {
  PalisadeOperations op(cc);
  return Base::Run(result, args, &op);
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
