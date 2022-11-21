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

#include "transpiler/openfhe_runner.h"

#include <vector>

#include "absl/status/statusor.h"
#include "absl/types/optional.h"
#include "openfhe/binfhe/binfhecontext.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

OpenFheCiphertext OpenFheRunner::OpenFheOperations::And(
    OpenFheCiphertextConstRef lhs, OpenFheCiphertextConstRef rhs) {
  if (lhs == rhs) {
    return lhs;
  }
  return cc_.EvalBinGate(lbcrypto::AND, lhs, rhs);
}
OpenFheCiphertext OpenFheRunner::OpenFheOperations::Or(
    OpenFheCiphertextConstRef lhs, OpenFheCiphertextConstRef rhs) {
  if (lhs == rhs) {
    return lhs;
  }
  return cc_.EvalBinGate(lbcrypto::OR, lhs, rhs);
}
OpenFheCiphertext OpenFheRunner::OpenFheOperations::Not(
    OpenFheCiphertextConstRef in) {
  return cc_.EvalNOT(in);
}

OpenFheCiphertext OpenFheRunner::OpenFheOperations::Constant(bool value) {
  return cc_.EvalConstant(value);
}

void OpenFheRunner::OpenFheOperations::Copy(OpenFheCiphertextConstRef src,
                                            OpenFheCiphertextRef& dst) {
  *dst = *src;
}

OpenFheCiphertext OpenFheRunner::OpenFheOperations::CopyOf(
    OpenFheCiphertextConstRef src) {
  return std::make_shared<lbcrypto::LWECiphertextImpl>(*src);
}

absl::Status OpenFheRunner::Run(
    absl::Span<OpenFheCiphertextRef> result,
    absl::flat_hash_map<std::string, absl::Span<OpenFheCiphertextConstRef>>
        in_args,
    absl::flat_hash_map<std::string, absl::Span<OpenFheCiphertextRef>>
        inout_args,
    lbcrypto::BinFHEContext cc) {
  OpenFheOperations op(cc);
  return Base::Run(result, in_args, inout_args, &op);
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
