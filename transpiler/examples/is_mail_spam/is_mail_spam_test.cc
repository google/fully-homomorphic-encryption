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

#include "is_mail_spam.h"

#include "gtest/gtest.h"

#include <cstring>
#include <string>

class is_mail_spam : public ::testing::Test {
 protected:
  void SetUp() override {
    mail_[0] = '\0';
  }

  void setMail(const std::string &mail) {
    ASSERT_LT(mail.size(), kMaxMailSize);
    std::memcpy(mail_, mail.data(), mail.size());
    mail_[mail.size()] = '\0';
  }

  bool isMailSpam() {
    const auto res = ::isMailSpam(mail_);
    if (res) {
      return true;
    }
    return false;
  }

 private:
  char mail_[kMaxMailSize] = {'\0',};
};

TEST_F(is_mail_spam, exact_match) {
  setMail("evil.url");
  ASSERT_TRUE(isMailSpam());
}

TEST_F(is_mail_spam, longer_contains_spam) {
  setMail("click evil.url!");
  ASSERT_TRUE(isMailSpam());
}

TEST_F(is_mail_spam, same_size_but_different_last_char) {
  setMail("evil.ura");
  ASSERT_FALSE(isMailSpam());
}

TEST_F(is_mail_spam, same_size_but_different_first_char) {
  setMail("Bvil.url");
  ASSERT_FALSE(isMailSpam());
}

TEST_F(is_mail_spam, same_size_but_different_char_in_the_middle) {
  setMail("evil.crl");
  ASSERT_FALSE(isMailSpam());
}

TEST_F(is_mail_spam, shorter_mail_than_spam) {
  setMail("evil.ur");
  ASSERT_FALSE(isMailSpam());
}

TEST_F(is_mail_spam, shorter_mail_than_spam_with_diff_char) {
  setMail("evil-ur");
  ASSERT_FALSE(isMailSpam());
}

TEST_F(is_mail_spam, longer_mail_does_not_contain_spam) {
  setMail("dvil.urlx");
  ASSERT_FALSE(isMailSpam());
}
