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

#include "calculator.h"

#include "gtest/gtest.h"

TEST(CalculatorTest, Sum) {
  Calculator st;
  int result = st.process(64, 45, '+');

  EXPECT_EQ(result, 109);
}

TEST(CalculatorTest, SumNegativeValue) {
  Calculator st;
  int result = st.process(64, -45, '+');

  EXPECT_EQ(result, 19);
}

TEST(CalculatorTest, SumTwoNegativeValues) {
  Calculator st;
  int result = st.process(-64, -45, '+');

  EXPECT_EQ(result, -109);
}

TEST(CalculatorTest, Subtraction) {
  Calculator st;
  int result = st.process(64, 45, '-');

  EXPECT_EQ(result, 19);
}

TEST(CalculatorTest, Multiplication) {
  Calculator st;
  int result = st.process(10, 20, '*');

  EXPECT_EQ(result, 200);
}
