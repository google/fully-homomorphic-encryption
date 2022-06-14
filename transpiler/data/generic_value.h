// Copyright 2022 Google LLC
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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_GENERIC_VALUE_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_GENERIC_VALUE_H_

#include "absl/types/span.h"

template <typename Type, class Sample, class BootstrappingKey>
using CopyFnT = void(absl::Span<const Sample> value,
                     const BootstrappingKey* key, absl::Span<Sample> out);

template <typename Type, class Sample, class PublicKey>
using UnencryptedFnT = void(absl::Span<const bool> value, const PublicKey* key,
                            absl::Span<Sample> out);

template <typename Type, class Sample, class SecretKey>
using EncryptFnT = void(absl::Span<const bool> value, const SecretKey* key,
                        absl::Span<Sample> out);

template <typename Type, class Sample, class SecretKey>
using DecryptFnT = void(absl::Span<const Sample> ciphertext,
                        const SecretKey* key, absl::Span<bool> plaintext);

template <typename Type, class Sample, class SampleArrayDeleter,
          class SecretKey, class PublicKey, class BootstrappingKey,
          CopyFnT<Type, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<Type, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<Type, Sample, SecretKey> EncryptFn,
          DecryptFnT<Type, Sample, SecretKey> DecryptFn>
class GenericEncodedRef;

template <typename Type, class Sample, class SampleArrayDeleter,
          class SecretKey, class PublicKey, class BootstrappingKey,
          CopyFnT<Type, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<Type, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<Type, Sample, SecretKey> EncryptFn,
          DecryptFnT<Type, Sample, SecretKey> DecryptFn>
class GenericEncoded;

template <typename Type, class Sample, class SampleArrayDeleter,
          class SecretKey, class PublicKey, class BootstrappingKey,
          CopyFnT<Type, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<Type, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<Type, Sample, SecretKey> EncryptFn,
          DecryptFnT<Type, Sample, SecretKey> DecryptFn, unsigned... Dimensions>
class GenericEncodedArray;

template <typename Type, class Sample, class SampleArrayDeleter,
          class SecretKey, class PublicKey, class BootstrappingKey,
          CopyFnT<Type, Sample, BootstrappingKey> CopyFn,
          UnencryptedFnT<Type, Sample, PublicKey> UnencryptedFn,
          EncryptFnT<Type, Sample, SecretKey> EncryptFn,
          DecryptFnT<Type, Sample, SecretKey> DecryptFn, unsigned... Dimensions>
class GenericEncodedArrayRef;

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_DATA_GENERIC_VALUE_H_
