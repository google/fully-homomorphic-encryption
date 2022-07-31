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

static bool containsSpam(char mail[kMaxMailSize], unsigned offset) {
  static constexpr auto kSpamSize = 8 + 1;
  const char spam_str[kSpamSize] = "evil.url";
  auto contains = true;

  #pragma hls_unroll yes
  for (auto i = 0u; i < kSpamSize - 1; ++i) {
    if (offset + i >= kMaxMailSize - 1 || mail[offset + i] != spam_str[i]) {
      contains = false;
      break;
    }
  }
  return contains;
}

#pragma hls_top
int isMailSpam(char mail[kMaxMailSize]) {
  auto contains = false;
  #pragma hls_unroll yes
  for (auto i = 0; i < kMaxMailSize; ++i) {
    if (mail[i] != '\0') {
      if (containsSpam(mail, i)) {
        contains = true;
        break;
      }
    }
  }
  return (contains ? 1 : 0);
}
