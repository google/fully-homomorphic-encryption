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

#include "transpiler/data/tfhe_data.h"

#include <cstring>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "include/ac_int.h"

constexpr int kMainMinimumLambda = 120;

TEST(TfheDataTest, TfheIntegers) {
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);
  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  auto bool_value = TfheInteger<1, false>::Encrypt(true, key);
  EXPECT_EQ(bool_value.Decrypt(key), true);
  auto char_value = TfheInteger<8, true>::Encrypt('t', key);
  EXPECT_EQ(char_value.Decrypt(key), 't');
  auto short_value = TfheInteger<16, true>::Encrypt(0x1234, key);
  EXPECT_EQ(short_value.Decrypt(key), 0x1234);
  auto int_value = TfheInteger<32, true>::Encrypt(0x12345678, key);
  EXPECT_EQ(int_value.Decrypt(key), 0x12345678);
  auto unsigned_byte_value = TfheInteger<8, false>::Encrypt(0x7b, key);
  EXPECT_EQ(unsigned_byte_value.Decrypt(key), 0x7b);
  auto signed_byte_value = TfheInteger<8, true>::Encrypt(0xab, key);
  EXPECT_EQ(signed_byte_value.Decrypt(key), (int8_t)0xab);

  ac_int<80, false> x;
  x.bit_fill_hex("a9876543210fedcba987");
  auto large_value = TfheInteger<80, false>::Encrypt(x, key);
  ac_int<80, false> decoded_large_value = large_value.Decrypt(key);
  EXPECT_EQ(decoded_large_value, x);
}

TEST(TfheDataTest, TfhePrimitives) {
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);
  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  auto bool_value = Tfhe<bool>::Encrypt(true, key);
  EXPECT_EQ(bool_value.Decrypt(key), true);
  auto char_value = Tfhe<char>::Encrypt('t', key);
  EXPECT_EQ(char_value.Decrypt(key), 't');
  auto short_value = Tfhe<short>::Encrypt(0x1234, key);
  EXPECT_EQ(short_value.Decrypt(key), 0x1234);
  auto int_value = Tfhe<int>::Encrypt(0x12345678, key);
  EXPECT_EQ(int_value.Decrypt(key), 0x12345678);
  auto unsigned_byte_value = TfheValue<uint8_t>::Encrypt(0x7b, key);
  EXPECT_EQ(unsigned_byte_value.Decrypt(key), 0x7b);
  auto signed_byte_value = TfheValue<int8_t>::Encrypt(0xab, key);
  EXPECT_EQ(signed_byte_value.Decrypt(key), (int8_t)0xab);
}

TEST(TfheDataTest, TfheArraysSizeCheck) {
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);
  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  auto int_array = TfheArray<int32_t>::Encrypt({1, 2, 3}, key);
  EXPECT_EQ(int_array.length(), 3);
  EXPECT_EQ(int_array.bit_width(), 3 * 32);
  EXPECT_EQ(int_array.get().size(), int_array.bit_width());

  auto int_array_ref = TfheArrayRef<int32_t>(int_array);
  EXPECT_EQ(int_array_ref.get().size(), int_array.bit_width());
}

TEST(TfheDataTest, TfheArrays) {
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);
  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  auto int_array = TfheArray<int32_t>::Encrypt({1, 2}, key);
  const std::vector<int> expected_int_array = {1, 2};
  auto decoded = int_array.Decrypt(key);
  for (int i = 0; i < expected_int_array.size(); i++) {
    EXPECT_EQ(decoded[i], expected_int_array[i]);
  }
}

TEST(TfheDataTest, TfheString) {
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);
  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  auto str = TfheArray<char>::Encrypt("test string", key);
  auto decoded = str.Decrypt(key);
  EXPECT_EQ(std::strncmp(decoded.c_str(), "test string", decoded.size()), 0);
}

TEST(TfheDataTest, TfheRefs) {
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);
  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  // Test creating a reference to a value, passing that reference around, and
  // assigning it to another value.
  Tfhe<int> int_val_a = Tfhe<int>::Encrypt(0x12345678, key);
  TfheRef<int> int_val_a_ref = int_val_a;
  TfheRef<int> int_val_b_ref = int_val_a_ref;
  Tfhe<int> int_val_b(params);
  int_val_b = int_val_b_ref;
  EXPECT_EQ(int_val_b.Decrypt(key), 0x12345678);

  // Test getting a reference to an element of an array, assigning it to antoher
  // reference, then back to a value, decrypting that and making sure the
  // decrypted value is the same as the original.
  TfheArray<int> int_array = TfheArray<int>::Encrypt({1, 2}, key);
  const std::vector<int> expected_int_array = {1, 2};
  auto decoded = int_array.Decrypt(key);
  for (int i = 0; i < expected_int_array.size(); i++) {
    TfheRef<int> el_ref = int_array[i];
    TfheRef<int> el_ref_b = el_ref;
    Tfhe<int> el(params);
    el = el_ref_b;
    EXPECT_EQ(decoded[i], expected_int_array[i]);
    EXPECT_EQ(el.Decrypt(key), expected_int_array[i]);
  }
}
