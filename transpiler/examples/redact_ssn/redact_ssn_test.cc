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

#include "redact_ssn.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(RedactSsnTest, RedactSsnBeginning) {
  char result[MAX_LENGTH] = "123456789 at the beginning";
  RedactSsn(result);

  ASSERT_STREQ(result, "********* at the beginning");
}

TEST(RedactSsnTest, RedactSsnMiddle) {
  char result[] = "redact 123456789 away";
  RedactSsn(result);

  ASSERT_STREQ(result, "redact ********* away");
}

TEST(RedactSsnTest, RedactSsnEnd) {
  char result[] = "redact away 123456789";
  RedactSsn(result);

  ASSERT_STREQ(result, "redact away *********");
}

TEST(RedactSsnTest, DontRedactSsn10Digits) {
  char result[] = "redact away 1234567890";
  RedactSsn(result);

  ASSERT_STREQ(result, "redact away 1234567890");
}

TEST(RedactSsnTest, DontRedactSsnDash) {
  char result[] = "redact away 123456789-";
  RedactSsn(result);

  ASSERT_STREQ(result, "redact away 123456789-");
}

TEST(RedactSsnTest, RedactSsnDashBeginning) {
  char result[MAX_LENGTH] = "123-45-6789 at the beginning";
  RedactSsn(result);

  ASSERT_STREQ(result, "***-**-**** at the beginning");
}

TEST(RedactSsnTest, RedactSsnDashMiddle) {
  char result[] = "redact 123-45-6789 away";
  RedactSsn(result);

  ASSERT_STREQ(result, "redact ***-**-**** away");
}

TEST(RedactSsnTest, RedactSsnDashEnd) {
  char result[] = "redact away 123-45-6789";
  RedactSsn(result);

  ASSERT_STREQ(result, "redact away ***-**-****");
}
