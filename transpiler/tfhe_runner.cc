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

#include "transpiler/tfhe_runner.h"

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/types/optional.h"
#include "tfhe/tfhe.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

namespace {

std::vector<TfheCiphertextRef> ToRefVector(absl::Span<LweSample> values) {
  std::vector<TfheCiphertextRef> value_refs;
  value_refs.reserve(values.size());
  for (absl::Span<LweSample>::size_type i = 0; i < values.size(); ++i) {
    value_refs.push_back(&values[i]);
  }
  return value_refs;
}

std::vector<TfheCiphertextConstRef> ToRefVector(
    absl::Span<const LweSample> values) {
  std::vector<TfheCiphertextConstRef> value_refs;
  value_refs.reserve(values.size());
  for (absl::Span<const LweSample>::size_type i = 0; i < values.size(); ++i) {
    value_refs.push_back(&values[i]);
  }
  return value_refs;
}

}  // namespace

TfheCiphertext TfheRunner::TfheOperations::And(TfheCiphertextConstRef lhs,
                                               TfheCiphertextConstRef rhs) {
  TfheCiphertext result(bk_->params);
  bootsAND(result.get(), lhs.get(), rhs.get(), bk_);
  return result;
}
TfheCiphertext TfheRunner::TfheOperations::Or(TfheCiphertextConstRef lhs,
                                              TfheCiphertextConstRef rhs) {
  TfheCiphertext result(bk_->params);
  bootsOR(result.get(), lhs.get(), rhs.get(), bk_);
  return result;
}
TfheCiphertext TfheRunner::TfheOperations::Not(TfheCiphertextConstRef in) {
  TfheCiphertext result(bk_->params);
  bootsNOT(result.get(), in.get(), bk_);
  return result;
}

TfheCiphertext TfheRunner::TfheOperations::Constant(bool value) {
  TfheCiphertext result(bk_->params);
  bootsCONSTANT(result.get(), value, bk_);
  return result;
}

void TfheRunner::TfheOperations::Copy(TfheCiphertextConstRef src,
                                      TfheCiphertextRef& dst) {
  bootsCOPY(dst.get(), src.get(), bk_);
}

TfheCiphertext TfheRunner::TfheOperations::CopyOf(TfheCiphertextConstRef src) {
  TfheCiphertext dst(bk_->params);
  bootsCOPY(dst.get(), src.get(), bk_);
  return dst;
}

absl::Status TfheRunner::Run(
    absl::Span<LweSample> result,
    absl::flat_hash_map<std::string, absl::Span<const LweSample>> in_args,
    absl::flat_hash_map<std::string, absl::Span<LweSample>> inout_args,
    const TFheGateBootstrappingCloudKeySet* bk) {
  std::vector<TfheCiphertextRef> result_ref_vec = ToRefVector(result);
  absl::Span<TfheCiphertextRef> result_ref = absl::MakeSpan(result_ref_vec);

  absl::flat_hash_map<std::string, std::vector<TfheCiphertextConstRef>>
      in_arg_ref_vecs;
  absl::flat_hash_map<std::string, absl::Span<TfheCiphertextConstRef>>
      in_arg_refs;
  for (const auto& [name, arg] : in_args) {
    in_arg_ref_vecs[name] = ToRefVector(arg);
    in_arg_refs[name] = absl::MakeSpan(in_arg_ref_vecs[name]);
  }

  absl::flat_hash_map<std::string, std::vector<TfheCiphertextRef>>
      inout_arg_ref_vecs;
  absl::flat_hash_map<std::string, absl::Span<TfheCiphertextRef>>
      inout_arg_refs;
  for (const auto& [name, arg] : inout_args) {
    inout_arg_ref_vecs[name] = ToRefVector(arg);
    inout_arg_refs[name] = absl::MakeSpan(inout_arg_ref_vecs[name]);
  }

  TfheOperations op(bk);
  return Base::Run(result_ref, in_arg_refs, inout_arg_refs, &op);
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
