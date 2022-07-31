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

#include <iostream>
#include <string>

#include "transpiler/data/boolean_data.h"
#include "transpiler/examples/is_mail_spam/is_mail_spam_bool.h"
#include "xls/common/logging/logging.h"

#include "is_mail_spam.h"

using namespace std;

int main(int argc, char** argv) {
  if (argc < 2) {
    cerr << "Error: missing mail" << endl;
    cerr << "Usage: is_mail_spam \"mail contents\"" << endl;
    return -1;
  }

  // Create an input (the email) on the client side.
  //
  // Make sure that the string buffer we pass is exactly `kMaxMailSize` bytes
  // long and the last character is always the null terminator. We need the
  // resize() and explicitly setting the last character to '\0' as EncodedString
  // consumes as many bytes as in the std::string, but without its internal null
  // terminator and we want to pass the terminator too to isMailSpam().
  auto mail = string{argv[1]};
  mail.resize(kMaxMailSize, '\0');
  mail[kMaxMailSize - 1] = '\0';

  // Encode the mail on the client side.
  auto encoded_email = EncodedString{mail};
  cout << "[Client] Mail to be checked for spam: \"" << mail << "\"" << endl;
  cout << "[Client] Mail encoding done" << endl << endl;

  // Perform mail spam detection on the server side.
  cout << "\t\t\t\t\t\t\t[Server] Computing..." << endl;
  auto encoded_result = EncodedInt{};
  XLS_CHECK_OK(isMailSpam(encoded_result.get(), encoded_email.get()));
  cout << "\t\t\t\t\t\t\t[Server] Computation done" << endl;

  // Decode the result on the client side.
  const auto is_spam = encoded_result.Decode();
  cout << "[Client] Decoded result: " << is_spam << endl;
  if (is_spam) {
    cout << "[Client] Mail is spam!" << endl;
  } else {
    cout << "[Client] Mail is not spam." << endl;
  }
}
