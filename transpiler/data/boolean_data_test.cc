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

#include "transpiler/data/boolean_data.h"

#include <cstring>
#include <type_traits>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(BooleanDataTest, EncodedPrimitives) {
  auto bool_value = EncodedBool(true);
  EXPECT_EQ(bool_value.Decode(), true);
  auto char_value = EncodedChar('t');
  EXPECT_EQ(char_value.Decode(), 't');
  auto short_value = EncodedShort(0x1234);
  EXPECT_EQ(short_value.Decode(), 0x1234);
  auto int_value = EncodedInt(0x12345678);
  EXPECT_EQ(int_value.Decode(), 0x12345678);
  auto unsigned_byte_value = EncodedValue<uint8_t>(0x7b);
  EXPECT_EQ(unsigned_byte_value.Decode(), 0x7b);
  auto signed_byte_value = EncodedValue<int8_t>(0xab);
  EXPECT_EQ(signed_byte_value.Decode(), (int8_t)0xab);
}

TEST(BooleanDataTest, EncodedArraysSizeCheck) {
  auto int_array = EncodedArray<int32_t>({1, 2, 3});
  EXPECT_EQ(int_array.length(), 3);
  EXPECT_EQ(int_array.bit_width(), 3 * 32);
  EXPECT_EQ(int_array.get().size(), int_array.bit_width());

  auto int_array_ref = EncodedArrayRef<int32_t>(int_array);
  EXPECT_EQ(int_array_ref.get().size(), int_array.bit_width());
}

TEST(BooleanDataTest, EncodedArrays) {
  auto int_array = EncodedArray<int32_t>({1, 2});
  const std::vector<int> expected_int_array = {1, 2};
  auto decoded = int_array.Decode();
  for (int i = 0; i < expected_int_array.size(); i++) {
    EXPECT_EQ(decoded[i], expected_int_array[i]);
  }
}

TEST(BooleanDataTest, EncodedString) {
  auto str = EncodedString("test string");
  auto decoded = str.Decode();
  EXPECT_EQ(std::strncmp(decoded.c_str(), "test string", decoded.size()), 0);
}

TEST(BooleanDataTest, EncodedRefs) {
  // Test creating a reference to a value, passing that reference around, and
  // assigning it to another value.
  EncodedInt int_val_a = EncodedInt(0x12345678);
  EncodedValueRef<int> int_val_a_ref = int_val_a;
  EncodedValueRef<int> int_val_b_ref = int_val_a_ref;
  EncodedInt int_val_b;
  int_val_b = int_val_b_ref;
  EXPECT_EQ(int_val_b.Decode(), 0x12345678);

  // Test getting a reference to an element of an array, assigning it to antoher
  // reference, then back to a value, decrypting that and making sure the
  // decrypted value is the same as the original.
  EncodedArray<int> int_array = EncodedArray<int>({1, 2});
  const std::vector<int> expected_int_array = {1, 2};
  auto decoded = int_array.Decode();
  for (int i = 0; i < expected_int_array.size(); i++) {
    EncodedValueRef<int> el_ref = int_array[i];
    EncodedValueRef<int> el_ref_b = el_ref;
    EncodedInt el;
    el = el_ref_b;
    EXPECT_EQ(decoded[i], expected_int_array[i]);
    EXPECT_EQ(el.Decode(), expected_int_array[i]);
  }
}
