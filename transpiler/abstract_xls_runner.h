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

// Funcionality for "interpreting" XLS IR within a parametrized environment.
//
// IR must satisfy:
//
// * Every data type is bits.
// * Only params and return values have width > 1.
// * The return value is a CONCAT node.

// Usage:
//
// auto runner = <APPROPRIATE>Runner::Create("/path/to/ir", "my_package");
// auto bk = ... set up keys ..
// auto result == ... set up output parameter with the right width ...
// auto args = absl::flat_hash_map {{"x", ...}, {"y", ...}};
// runner.Run(result, args, bk);

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_ABSTRACT_XLS_RUNNER_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_ABSTRACT_XLS_RUNNER_H_

#include <pthread.h>
#include <semaphore.h>

#include <memory>
#include <queue>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/protobuf/text_format.h"
#include "transpiler/abstract_xls_transpiler.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/metadata_output.pb.h"
#include "xls/public/ir.h"
#include "xls/public/ir_parser.h"

namespace fully_homomorphic_encryption {
namespace transpiler {

template <typename Derived, typename EncodedBit,
          typename EncodedBitRef = EncodedBit,
          typename EncodedBitConstRef = const EncodedBitRef>
class AbstractXlsRunner {
 protected:
  struct BitOperations {
    virtual EncodedBit And(EncodedBitConstRef lhs, EncodedBitConstRef rhs) = 0;
    virtual EncodedBit Or(EncodedBitConstRef lhs, EncodedBitConstRef rhs) = 0;
    virtual EncodedBit Not(EncodedBitConstRef in) = 0;

    virtual EncodedBit Constant(bool value) = 0;

    virtual void Copy(EncodedBitConstRef src, EncodedBitRef& dst) = 0;

    virtual EncodedBit CopyOf(EncodedBitConstRef in) = 0;
  };

 public:
  AbstractXlsRunner(std::unique_ptr<xls::Package> package,
                    xlscc_metadata::MetadataOutput metadata);
  ~AbstractXlsRunner();

  absl::Status Run(
      absl::Span<EncodedBitRef> result,
      absl::flat_hash_map<std::string, absl::Span<EncodedBitConstRef>> in_args,
      absl::flat_hash_map<std::string, absl::Span<EncodedBitRef>> inout_args,
      BitOperations* op);

  static absl::StatusOr<std::unique_ptr<Derived>> CreateFromFile(
      absl::string_view ir_path, absl::string_view metadata_path);

  static absl::StatusOr<std::unique_ptr<Derived>> CreateFromStrings(
      absl::string_view xls_package, absl::string_view metadata_text);

 private:
  absl::StatusOr<xls::Function*> GetEntry() {
    return package_->GetFunction(metadata_.top_func_proto().name().name());
  }

  // This method copies the relevant bit from input params into the result.
  static absl::StatusOr<EncodedBit> HandleBitSlice(
      const xls::BitSlice* bit_slice,
      absl::flat_hash_map<std::string, absl::Span<EncodedBitConstRef>> in_args,
      absl::flat_hash_map<std::string, absl::Span<EncodedBitRef>> inout_args,
      BitOperations* op);

  // Array support will need to be updated when structs are added: it could be
  // possible that there is padding present between subsequent elements in an
  // array of these structs that is not captured by the corresponding XLS type -
  // for example, a 56-byte struct will likely be padded out to 64 bytes
  // internally. This code would assume that struct data is all packed, and thus
  // the output would be garbled. Host layout will need to be considered here.
  absl::Status CollectNodeValue(
      const xls::Node* node, absl::Span<EncodedBitRef> output_arg,
      int output_offset,
      const absl::flat_hash_map</*id=*/uint64_t, absl::optional<EncodedBit>>&
          values);

  // Walks the type elements comprising `function`'s output type and generates
  // FHE copy operations to extract the data corresponding to each.
  //
  // At present, `function`'s output must be of the form (A, B), where A is
  // bits- or array-typed, and B must be a tuple containing only bits- or
  // array-typed elements. A corresponds to the output from the original C++
  // function itself, and the elements of B are the in/out params to the
  // function. We don't currently have the ability to traverse the definition of
  // any given [C/C++] struct, so struct/tuple types are not _currently_
  // supported, though this is intended to change in the near future.
  absl::Status CollectOutputs(
      absl::Span<EncodedBitRef> result,
      absl::flat_hash_map<std::string, absl::Span<EncodedBitRef>> inout_args,
      const absl::flat_hash_map</*id=*/uint64_t, absl::optional<EncodedBit>>&
          values);

  static void* ThreadBodyStatic(void* runner);
  absl::Status ThreadBody();

  // This is static to ensure no access to lock-protected state
  // Can return nullopt for no-ops
  static absl::StatusOr<absl::optional<EncodedBit>> EvalSingleOp(
      xls::Node* n, std::vector<absl::optional<EncodedBitConstRef>> operands,
      const absl::flat_hash_map<std::string, absl::Span<EncodedBitConstRef>>
          in_args,
      const absl::flat_hash_map<std::string, absl::Span<EncodedBitRef>>
          inout_args,
      BitOperations* op);

  absl::flat_hash_map<std::string, absl::Span<EncodedBitConstRef>>
      const_in_args_;
  absl::flat_hash_map<std::string, absl::Span<EncodedBitRef>> const_inout_args_;
  BitOperations* const_op_ = nullptr;

  typedef std::tuple<xls::Node*,
                     std::vector<absl::optional<EncodedBitConstRef>>>
      NodeToEval;

  pthread_mutex_t lock_;  // Only used by worker threads

  sem_t input_sem_;
  std::queue<NodeToEval> input_queue_;  // Protected by lock_ in worker threads

  typedef std::tuple<xls::Node*, absl::optional<EncodedBit>> NodeFromEval;
  sem_t output_sem_;
  std::queue<NodeFromEval>
      output_queue_;  // Protected by lock_ in worker threads

  std::atomic<bool> threads_should_exit_;

  std::unique_ptr<xls::Package> package_;
  std::string function_name_;
  std::vector<pthread_t> threads_;
  xlscc_metadata::MetadataOutput metadata_;
};

template <typename Derived, typename EncodedBit, typename EncodedBitRef,
          typename EncodedBitConstRef>
AbstractXlsRunner<Derived, EncodedBit, EncodedBitRef, EncodedBitConstRef>::
    AbstractXlsRunner(std::unique_ptr<xls::Package> package,
                      xlscc_metadata::MetadataOutput metadata)
    : package_(std::move(package)), metadata_(metadata) {
  threads_should_exit_.store(false);

  XLS_CHECK_EQ(0, pthread_mutex_init(&lock_, nullptr));
  XLS_CHECK_EQ(0, sem_init(&input_sem_, 1, 0));
  XLS_CHECK_EQ(0, sem_init(&output_sem_, 1, 0));

  // *2 for hyperthreading opportunities
  const int numCPU = sysconf(_SC_NPROCESSORS_ONLN) * 2;
  for (int c = 0; c < numCPU; ++c) {
    pthread_t new_thread;
    XLS_CHECK_EQ(
        0, pthread_create(&new_thread, nullptr,
                          AbstractXlsRunner::ThreadBodyStatic, (void*)this));
    threads_.push_back(new_thread);
  }
}

template <typename Derived, typename EncodedBit, typename EncodedBitRef,
          typename EncodedBitConstRef>
AbstractXlsRunner<Derived, EncodedBit, EncodedBitRef,
                  EncodedBitConstRef>::~AbstractXlsRunner() {
  threads_should_exit_.store(true);

  // Wake up threads
  for (pthread_t pt : threads_) {
    (void)pt;
    sem_post(&input_sem_);
  }
  // Wait for exit
  for (pthread_t pt : threads_) {
    pthread_join(pt, nullptr);
  }

  XLS_CHECK_EQ(0, pthread_mutex_destroy(&lock_));
  XLS_CHECK_EQ(0, sem_destroy(&input_sem_));
  XLS_CHECK_EQ(0, sem_destroy(&output_sem_));
}

template <typename Derived, typename EncodedBit, typename EncodedBitRef,
          typename EncodedBitConstRef>
absl::StatusOr<EncodedBit>
AbstractXlsRunner<Derived, EncodedBit, EncodedBitRef, EncodedBitConstRef>::
    HandleBitSlice(
        const xls::BitSlice* bit_slice,
        absl::flat_hash_map<std::string, absl::Span<EncodedBitConstRef>>
            in_args,
        absl::flat_hash_map<std::string, absl::Span<EncodedBitRef>> inout_args,
        BitOperations* op) {
  xls::Node* operand = bit_slice->operand(0);

  int slice_idx = bit_slice->start();

  for (; !operand->Is<xls::Param>(); operand = operand->operand(0)) {
    if (operand->Is<xls::TupleIndex>()) {
      int slice_idx_adj = 0;
      xls::TupleIndex* tuple_index = operand->As<xls::TupleIndex>();
      int64_t actual_tuple_index = tuple_index->index();
      xls::TupleType* tuple =
          tuple_index->operand(0)->GetType()->AsTupleOrDie();
      for (int i = 0; i < actual_tuple_index; i++) {
        slice_idx_adj += tuple->element_type(i)->GetFlatBitCount();
      }
      slice_idx += slice_idx_adj;
    } else if (operand->Is<xls::ArrayIndex>()) {
      // If we're slicing into an array index, then we just need to get
      // the bit offset of index in the bit vector that represents the array
      // (in booleanified space).
      const xls::ArrayIndex* array_index = operand->As<xls::ArrayIndex>();
      XLS_ASSIGN_OR_RETURN(const xls::ArrayType* array_type,
                           array_index->array()->GetType()->AsArray());

      // TODO: Only literal indices into single-dimensional arrays
      // are currently supported. To extend past 1-d, we'll need to walk up the
      // array index chain, determining at each step the offset from element 0,
      // and pass that back down here.
      absl::Span<xls::Node* const> indices = array_index->indices();
      // TODO: Only single-dimensional indexes are supported at the
      // moment.
      if (indices.size() != 1) {
        return absl::InvalidArgumentError(
            "Only single-dimensional arrays/array indices are supported.");
      }
      if (!indices[0]->Is<xls::Literal>()) {
        return absl::InvalidArgumentError(
            "Only literal indexes into arrays are supported.");
      }
      xls::Literal* literal = indices[0]->As<xls::Literal>();

      XLS_ASSIGN_OR_RETURN(int64_t concrete_index,
                           literal->value().bits().ToUint64());
      slice_idx +=
          array_type->element_type()->GetFlatBitCount() * concrete_index;
    }

    // Verify that the only things allowed in a BitSlice chain are array
    // indexes, tuple indexes, other bit slices, and the eventual params.
    XLS_CHECK(operand->Is<xls::ArrayIndex>() || operand->Is<xls::BitSlice>() ||
              operand->Is<xls::Param>() || operand->Is<xls::TupleIndex>())
        << "Invalid BitSlice operand: " << operand->ToString();
  }

  std::string param_name = operand->GetName();
  auto found_in_arg = in_args.find(param_name);
  if (found_in_arg != in_args.end()) {
    return op->CopyOf(found_in_arg->second[slice_idx]);
  }
  auto found_inout_arg = inout_args.find(param_name);
  XLS_CHECK(found_inout_arg != inout_args.end());
  return op->CopyOf(found_inout_arg->second[slice_idx]);
}

template <typename Derived, typename EncodedBit, typename EncodedBitRef,
          typename EncodedBitConstRef>
absl::Status
AbstractXlsRunner<Derived, EncodedBit, EncodedBitRef, EncodedBitConstRef>::
    CollectNodeValue(
        const xls::Node* node, absl::Span<EncodedBitRef> output_arg,
        int output_offset,
        const absl::flat_hash_map</*id=*/uint64_t, absl::optional<EncodedBit>>&
            values) {
  xls::Type* type = node->GetType();
  std::string outputs;
  switch (type->kind()) {
    case xls::TypeKind::kBits: {
      // If this is a single bit, then we can [finally] emit the copy.
      int64_t bit_count = type->GetFlatBitCount();
      if (bit_count == 1) {
        // We can't handle concats in the transpiler, so if our single-bit is
        // one, walk up a level.
        while (node->Is<xls::Concat>()) {
          node = node->operand(0);
        }
        // Copy this node to the appropriate bit of the output.
        const_op_->Copy(*values.at(node->id()), output_arg[output_offset]);
        break;
      }

      // Otherwise, keep drilling down. Note that we iterate over bits in
      // "reverse" order, to match XLS' internal big-endian bit ordering (NOT
      // BYTE ORDERING) to the currently assumed little-endian bit ordering of
      // the host.
      for (int i = 0; i < bit_count; i++) {
        XLS_RETURN_IF_ERROR(
            CollectNodeValue(node->operand(i), output_arg,
                             output_offset + (bit_count - i - 1), values));
      }
      break;
    }
    case xls::TypeKind::kArray: {
      const xls::ArrayType* array_type = type->AsArrayOrDie();
      int64_t stride = array_type->element_type()->GetFlatBitCount();
      for (int i = 0; i < array_type->size(); i++) {
        XLS_RETURN_IF_ERROR(CollectNodeValue(
            node->operand(i), output_arg, output_offset + i * stride, values));
      }
      break;
    }
    case xls::TypeKind::kTuple: {
      // TODO: Populating output tuple types can be dangerous -
      // if they correspond to C structures, then there could be strange
      // issues such as host-native structure layout not matching the packed
      // layout used inside XLS, e.g.,
      // struct Foo {
      //  char a;
      //  short b;
      //  int c;
      // };
      // may have padding inserted around some elements. User beware (for now,
      // at least).
      const xls::TupleType* tuple_type = type->AsTupleOrDie();
      int64_t sub_offset = 0;
      for (int i = 0; i < tuple_type->size(); i++) {
        XLS_RETURN_IF_ERROR(CollectNodeValue(
            node->operand(i), output_arg, output_offset + sub_offset, values));
        sub_offset += node->operand(i)->GetType()->GetFlatBitCount();
      }
      break;
    }
    default:
      return absl::InvalidArgumentError(
          absl::StrCat("Unsupported type kind: ", type->kind()));
  }
  return absl::OkStatus();
}

template <typename Derived, typename EncodedBit, typename EncodedBitRef,
          typename EncodedBitConstRef>
absl::Status
AbstractXlsRunner<Derived, EncodedBit, EncodedBitRef, EncodedBitConstRef>::
    CollectOutputs(
        absl::Span<EncodedBitRef> result,
        absl::flat_hash_map<std::string, absl::Span<EncodedBitRef>> inout_args,
        const absl::flat_hash_map</*id=*/uint64_t, absl::optional<EncodedBit>>&
            values) {
  XLS_ASSIGN_OR_RETURN(auto function, GetEntry());
  const xls::Node* return_value = function->return_value();

  std::vector<const xls::Node*> elements;
  const xls::Type* type = return_value->GetType();

  // The return value can be a tuple in two cases:
  //
  // Case A:
  //    StructA foo(StructB in)
  //    void foo(Struct &in)
  // Case B:
  //    StructR foo(StructA a, StructB &b)
  //    StructR foo(StructA &a, StructB b)
  //    void foo(StructA &a, StructB &b)
  //
  // IOW, Case A is when a function returns a single struct (either explicitly
  // or as an non-const reference, this struct is represented as a tuple in the
  // return type.  CaseB is when multiple values are returned (via some
  // combination of a return value and one or more non-const references, or a
  // void return and two or more non-const references.
  //
  // Thus in case A we want to append to elements, while in case B we want to
  // splice the results tuple in.
  //
  // In both cases, type->kind() will be a kTuple, so we need an additional
  // check to tell cases A and B apart.

  auto top_func_proto = metadata_.top_func_proto();
  auto params = top_func_proto.params();
  auto return_type = top_func_proto.return_type();

  int num_out_params = xlscc_metadata::GetNumOutParams(metadata_);

  if (type->kind() == xls::TypeKind::kTuple) {
    if (num_out_params != 1) {
      elements.insert(elements.begin(), return_value->operands().begin(),
                      return_value->operands().end());
    } else {
      elements.push_back(return_value);
    }
  } else {
    elements.push_back(return_value);
  }

  if (elements.empty()) {
    return absl::OkStatus();
  }

  int output_idx = 0;
  if (metadata_.top_func_proto().return_type().has_as_void()) {
    if (!result.empty()) {
      return absl::FailedPreconditionError(
          "return value requested for a void-returning function");
    }
  } else {
    if (result.empty()) {
      return absl::FailedPreconditionError(
          "missing return value for a value-returning function");
    }
    XLS_RETURN_IF_ERROR(
        CollectNodeValue(elements[output_idx++], result, 0, values));
  }

  const auto& fn_params = metadata_.top_func_proto().params();
  int param_idx = 0;
  for (; output_idx < elements.size(); output_idx++) {
    const xlscc_metadata::FunctionParameter* param;
    while (true) {
      if (param_idx == fn_params.size()) {
        return absl::InternalError(absl::StrCat(
            "No matching in/out param for output %d: ", output_idx));
      }

      param = &fn_params[param_idx++];
      if (!param->is_const() && param->is_reference()) {
        break;
      }

      if (!param->has_type()) {
        return absl::InternalError(
            absl::StrCat("Parameter %s has no type.", param->name()));
      }
    }

    XLS_RETURN_IF_ERROR(CollectNodeValue(elements[output_idx],
                                         inout_args[param->name()], 0, values));
  }

  return absl::OkStatus();
}

template <typename Derived, typename EncodedBit, typename EncodedBitRef,
          typename EncodedBitConstRef>
absl::StatusOr<std::unique_ptr<Derived>> AbstractXlsRunner<
    Derived, EncodedBit, EncodedBitRef,
    EncodedBitConstRef>::CreateFromFile(absl::string_view ir_path,
                                        absl::string_view metadata_path) {
  XLS_ASSIGN_OR_RETURN(std::string ir_text, xls::GetFileContents(ir_path));
  XLS_ASSIGN_OR_RETURN(auto package, xls::ParsePackage(ir_text, std::nullopt));

  XLS_ASSIGN_OR_RETURN(std::string metadata_binary,
                       xls::GetFileContents(metadata_path));
  xlscc_metadata::MetadataOutput metadata;
  if (!metadata.ParseFromString(metadata_binary)) {
    return absl::InvalidArgumentError(
        "Could not parse function metadata proto.");
  }
  return std::make_unique<Derived>(std::move(package), std::move(metadata));
}

template <typename Derived, typename EncodedBit, typename EncodedBitRef,
          typename EncodedBitConstRef>
absl::StatusOr<std::unique_ptr<Derived>> AbstractXlsRunner<
    Derived, EncodedBit, EncodedBitRef,
    EncodedBitConstRef>::CreateFromStrings(absl::string_view xls_package,
                                           absl::string_view metadata_text) {
  XLS_ASSIGN_OR_RETURN(
      auto package, xls::ParsePackage(xls_package, /*filename=*/std::nullopt));

  xlscc_metadata::MetadataOutput metadata;
  if (!google::protobuf::TextFormat::ParseFromString(std::string(metadata_text),
                                                     &metadata)) {
    return absl::InvalidArgumentError(
        "Could not parse function metadata proto.");
  }

  return std::make_unique<Derived>(std::move(package), std::move(metadata));
}

template <typename Derived, typename EncodedBit, typename EncodedBitRef,
          typename EncodedBitConstRef>
absl::StatusOr<absl::optional<EncodedBit>>
AbstractXlsRunner<Derived, EncodedBit, EncodedBitRef, EncodedBitConstRef>::
    EvalSingleOp(
        xls::Node* n, std::vector<absl::optional<EncodedBitConstRef>> operands,
        const absl::flat_hash_map<std::string, absl::Span<EncodedBitConstRef>>
            in_args,
        const absl::flat_hash_map<std::string, absl::Span<EncodedBitRef>>
            inout_args,
        BitOperations* op) {
  XLS_CHECK(n != nullptr);
  auto node_type = n->op();
  if (node_type == xls::Op::kArray || node_type == xls::Op::kArrayIndex ||
      node_type == xls::Op::kConcat || node_type == xls::Op::kParam ||
      node_type == xls::Op::kShrl || node_type == xls::Op::kTuple ||
      node_type == xls::Op::kTupleIndex) {
    // These are all handled as operands to slice nodes.
    return absl::nullopt;
  }

  switch (node_type) {
    case xls::Op::kBitSlice: {
      // Slices should be of parameters with width 1.
      auto slice = n->As<xls::BitSlice>();
      return HandleBitSlice(slice, in_args, inout_args, op);
    } break;
    case xls::Op::kLiteral: {
      // Literals must be bits with width 1, or else used purely as array
      // indices.
      auto literal = n->As<xls::Literal>();
      auto bits = literal->GetType()->AsBitsOrDie();
      if (bits->bit_count() == 1) {
        return op->Constant(!literal->value().IsAllZeros());
      } else {
        // We allow literals strictly for pulling values out of [param]
        // arrays.
        for (const xls::Node* user : literal->users()) {
          if (!user->Is<xls::ArrayIndex>()) {
            XLS_LOG(FATAL) << "Unsupported literal: " << n->ToString();
          }
        }
        return absl::nullopt;
      }
    } break;
    case xls::Op::kAnd: {
      XLS_CHECK_EQ(operands.size(), 2);
      XLS_CHECK(operands[0].has_value());
      XLS_CHECK(operands[1].has_value());
      return op->And(*operands[0], *operands[1]);
    } break;
    case xls::Op::kOr: {
      XLS_CHECK_EQ(operands.size(), 2);
      XLS_CHECK(operands[0].has_value());
      XLS_CHECK(operands[1].has_value());
      return op->Or(*operands[0], *operands[1]);
    } break;
    case xls::Op::kNot: {
      XLS_CHECK_EQ(operands.size(), 1);
      XLS_CHECK(operands[0].has_value());
      return op->Not(*operands[0]);
    } break;
    default:
      XLS_LOG(FATAL) << "Unsupported node: " << n->ToString();
      return nullptr;
  }
}

template <typename Derived, typename EncodedBit, typename EncodedBitRef,
          typename EncodedBitConstRef>
absl::Status
AbstractXlsRunner<Derived, EncodedBit, EncodedBitRef, EncodedBitConstRef>::Run(
    absl::Span<EncodedBitRef> result,
    absl::flat_hash_map<std::string, absl::Span<EncodedBitConstRef>> in_args,
    absl::flat_hash_map<std::string, absl::Span<EncodedBitRef>> inout_args,
    BitOperations* op) {
  XLS_CHECK(input_queue_.empty());
  XLS_CHECK(output_queue_.empty());

  const_in_args_ = in_args;
  const_inout_args_ = inout_args;
  const_op_ = op;

  XLS_ASSIGN_OR_RETURN(auto entry, GetEntry());
  auto type = entry->GetType();

  // Arguments must match and all types must be bits.
  XLS_CHECK(type->parameter_count() == in_args.size() + inout_args.size());
  for (auto n : entry->params()) {
    XLS_CHECK(n != nullptr);
    XLS_CHECK(in_args.contains(n->name()) || inout_args.contains(n->name()));
  }

  auto return_value = entry->return_value();
  XLS_CHECK(return_value != nullptr);

  // Map of intermediate bits of ciphertext, indexed by node id.
  absl::flat_hash_map<uint64_t, absl::optional<EncodedBit>> values;

  absl::flat_hash_set<xls::Node*> unevaluated;
  unevaluated.insert(entry->nodes().begin(), entry->nodes().end());

  while (!unevaluated.empty()) {
    // Threads should not be running right now
    XLS_CHECK(input_queue_.empty());
    XLS_CHECK(output_queue_.empty());

    // Scan ahead and find nodes that are ready to be evaluated
    for (xls::Node* n : unevaluated) {
      std::vector<absl::optional<EncodedBitConstRef>> operands;
      // operands.resize(n->operand_count());

      bool all_operands_ready = true;

      for (int opi = 0; opi < n->operand_count(); ++opi) {
        xls::Node* opn = n->operand(opi);
        auto found_val = values.find(opn->id());
        if (found_val == values.end()) {
          all_operands_ready = false;
          break;
        }
        operands.emplace_back(found_val->second);
      }

      if (all_operands_ready) {
        input_queue_.push(NodeToEval(n, operands));
      }
    }

    const int n_to_run = input_queue_.size();

    // Unblock the worker threads
    for (int i = 0; i < n_to_run; ++i) {
      sem_post(&input_sem_);
    }

    // Wait for output from the worker threads
    for (int i = 0; i < n_to_run; ++i) {
      sem_wait(&output_sem_);
    }

    // Process output
    while (!output_queue_.empty()) {
      NodeFromEval from_eval = std::move(output_queue_.front());
      output_queue_.pop();

      xls::Node* n = std::get<0>(from_eval);

      // Even if the result was nullptr, mark the op as complete
      XLS_CHECK(!values.contains(n->id()));
      values[n->id()] = std::move(std::get<1>(from_eval));

      unevaluated.erase(n);
    }
  }

  // Copy the return value.
  XLS_RETURN_IF_ERROR(CollectOutputs(result, inout_args, values));

  const_in_args_.clear();
  const_inout_args_.clear();
  const_op_ = nullptr;
  return absl::OkStatus();
}

template <typename Derived, typename EncodedBit, typename EncodedBitRef,
          typename EncodedBitConstRef>
void* AbstractXlsRunner<Derived, EncodedBit, EncodedBitRef,
                        EncodedBitConstRef>::ThreadBodyStatic(void* runner) {
  XLS_CHECK(reinterpret_cast<Derived*>(runner)->ThreadBody().ok());
  return 0;
}

template <typename Derived, typename EncodedBit, typename EncodedBitRef,
          typename EncodedBitConstRef>
absl::Status AbstractXlsRunner<Derived, EncodedBit, EncodedBitRef,
                               EncodedBitConstRef>::ThreadBody() {
  while (true) {
    // Wait for the signal from the main thread
    sem_wait(&input_sem_);

    // Check if the signal is to exit
    if (threads_should_exit_.load()) {
      return absl::OkStatus();
    }

    // Get an input safely
    pthread_mutex_lock(&lock_);
    NodeToEval to_eval = input_queue_.front();
    input_queue_.pop();
    pthread_mutex_unlock(&lock_);

    // Process the input
    xls::Node* n = std::get<0>(to_eval);
    absl::optional<EncodedBit> out = absl::nullopt;
    XLS_ASSIGN_OR_RETURN(out,
                         EvalSingleOp(n, std::get<1>(to_eval), const_in_args_,
                                      const_inout_args_, const_op_));

    // Save the output safely
    pthread_mutex_lock(&lock_);
    output_queue_.push(NodeFromEval(n, std::move(out)));
    pthread_mutex_unlock(&lock_);

    // Signal the main thread
    sem_post(&output_sem_);
  }
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_ABSTRACT_XLS_RUNNER_H_
