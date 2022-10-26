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

#include "string_cap_char.h"

State::State() : last_was_space_(true) {}

unsigned char State::process(unsigned char c) {
  unsigned char ret = c;

  if (last_was_space_ && (c >= 'a') && (c <= 'z')) ret -= ('a' - 'A');

  last_was_space_ = (c == ' ');
  return ret;
}

#pragma hls_top
char my_package(State &st, char c) { return st.process(c); }
