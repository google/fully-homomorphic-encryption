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

#include "transpiler/data/openfhe_data.h"

#include <cstring>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "include/ac_int.h"

constexpr auto kSecurityLevel = lbcrypto::MEDIUM;

TEST(OpenFheDataTest, OpenFheIntegers) {
  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  auto bool_value = OpenFheInteger<1, false>::Encrypt(true, cc, sk);
  EXPECT_EQ(bool_value.Decrypt(sk), true);
  auto char_value = OpenFheInteger<8, true>::Encrypt('t', cc, sk);
  EXPECT_EQ(char_value.Decrypt(sk), 't');
  auto short_value = OpenFheInteger<16, true>::Encrypt(0x1234, cc, sk);
  EXPECT_EQ(short_value.Decrypt(sk), 0x1234);
  auto int_value = OpenFheInteger<32, true>::Encrypt(0x12345678, cc, sk);
  EXPECT_EQ(int_value.Decrypt(sk), 0x12345678);
  auto unsigned_byte_value = OpenFheInteger<8, false>::Encrypt(0x7b, cc, sk);
  EXPECT_EQ(unsigned_byte_value.Decrypt(sk), 0x7b);
  auto signed_byte_value = OpenFheInteger<8, true>::Encrypt(0xab, cc, sk);
  EXPECT_EQ(signed_byte_value.Decrypt(sk), (int8_t)0xab);

  ac_int<80, false> x;
  x.bit_fill_hex("a9876543210fedcba987");
  auto large_value = OpenFheInteger<80, false>::Encrypt(x, cc, sk);
  ac_int<80, false> decoded_large_value = large_value.Decrypt(sk);
  EXPECT_EQ(decoded_large_value, x);
}

TEST(OpenFheDataTest, OpenFhePrimitives) {
  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  auto bool_value = OpenFhe<bool>::Encrypt(true, cc, sk);
  EXPECT_EQ(bool_value.Decrypt(sk), true);
  auto char_value = OpenFhe<char>::Encrypt('t', cc, sk);
  EXPECT_EQ(char_value.Decrypt(sk), 't');
  auto short_value = OpenFhe<short>::Encrypt(0x1234, cc, sk);
  EXPECT_EQ(short_value.Decrypt(sk), 0x1234);
  auto int_value = OpenFhe<int>::Encrypt(0x12345678, cc, sk);
  EXPECT_EQ(int_value.Decrypt(sk), 0x12345678);
  auto unsigned_byte_value = OpenFheValue<uint8_t>::Encrypt(0x7b, cc, sk);
  EXPECT_EQ(unsigned_byte_value.Decrypt(sk), 0x7b);
  auto signed_byte_value = OpenFheValue<int8_t>::Encrypt(0xab, cc, sk);
  EXPECT_EQ(signed_byte_value.Decrypt(sk), (int8_t)0xab);
}

TEST(OpenFheDataTest, OpenFheArraysSizeCheck) {
  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  auto int_array = OpenFheArray<int32_t>::Encrypt({1, 2, 3}, cc, sk);
  EXPECT_EQ(int_array.length(), 3);
  EXPECT_EQ(int_array.bit_width(), 3 * 32);
  EXPECT_EQ(int_array.get().size(), int_array.bit_width());

  auto int_array_ref = OpenFheArrayRef<int32_t>(int_array);
  EXPECT_EQ(int_array_ref.get().size(), int_array.bit_width());
}

TEST(OpenFheDataTest, OpenFheArrays) {
  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  auto int_array = OpenFheArray<int32_t>::Encrypt({1, 2}, cc, sk);
  const std::vector<int> expected_int_array = {1, 2};
  auto decoded = int_array.Decrypt(sk);
  for (int i = 0; i < expected_int_array.size(); i++) {
    EXPECT_EQ(decoded[i], expected_int_array[i]);
  }
}

TEST(OpenFheDataTest, OpenFheString) {
  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  auto str = OpenFheArray<char>::Encrypt("test string", cc, sk);
  auto decoded = str.Decrypt(sk);
  EXPECT_EQ(std::strncmp(decoded.c_str(), "test string", decoded.size()), 0);
}

TEST(OpenFheDataTest, OpenFheRefs) {
  // generate a keyset
  auto cc = lbcrypto::BinFHEContext();

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  cc.GenerateBinFHEContext(kSecurityLevel);
  auto sk = cc.KeyGen();
  cc.BTKeyGen(sk);

  // Test creating a reference to a value, passing that reference around, and
  // assigning it to another value.
  OpenFhe<int> int_val_a = OpenFhe<int>::Encrypt(0x12345678, cc, sk);
  OpenFheRef<int> int_val_a_ref = int_val_a;
  OpenFheRef<int> int_val_b_ref = int_val_a_ref;
  OpenFhe<int> int_val_b(cc);
  int_val_b = int_val_b_ref;
  EXPECT_EQ(int_val_b.Decrypt(sk), 0x12345678);

  // Test getting a reference to an element of an array, assigning it to antoher
  // reference, then back to a value, decrypting that and making sure the
  // decrypted value is the same as the original.
  OpenFheArray<int> int_array = OpenFheArray<int>::Encrypt({1, 2}, cc, sk);
  const std::vector<int> expected_int_array = {1, 2};
  auto decoded = int_array.Decrypt(sk);
  for (int i = 0; i < expected_int_array.size(); i++) {
    OpenFheRef<int> el_ref = int_array[i];
    OpenFheRef<int> el_ref_b = el_ref;
    OpenFhe<int> el(cc);
    el = el_ref_b;
    EXPECT_EQ(decoded[i], expected_int_array[i]);
    EXPECT_EQ(el.Decrypt(sk), expected_int_array[i]);
  }
}
