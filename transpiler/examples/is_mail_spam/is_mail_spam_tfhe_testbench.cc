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

#include <stdint.h>

#include <iostream>
#include <string>

#include "tfhe/tfhe.h"
#include "transpiler/data/fhe_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_TFHE
#include "transpiler/examples/is_mail_spam/is_mail_spam_interpreted_tfhe.h"
#else
#include "transpiler/examples/is_mail_spam/is_mail_spam_tfhe.h"
#endif

#include "is_mail_spam.h"

static const auto main_minimum_lambda = 120;

using namespace std;

int main(int argc, char** argv) {
  if (argc < 2) {
    cerr << "Error: missing mail" << endl;
    cerr << "Usage: is_mail_spam \"mail contents\"" << endl;
    return -1;
  }

  // Generate a keyset.
  auto params = new_default_gate_bootstrapping_parameters(main_minimum_lambda);

  // Generate a random key.
  // Note: In real applications, a cryptographically secure seed needs to be used.
  uint32_t seed[] = {314, 1592, 657};
  tfhe_random_generator_setSeed(seed, 3);
  const auto key = new_random_gate_bootstrapping_secret_keyset(params);
  const auto cloud_key = &key->cloud;

  // Create an input (the email) on the client side.
  //
  // Make sure that the string buffer we pass is exactly `kMaxMailSize` bytes
  // long and the last character is always the null terminator. We need the
  // resize() and explicitly setting the last character to '\0' as FheString::Encrypt()
  // consumes as many bytes as in the std::string, but without its internal null
  // terminator and we want to pass the terminator too to isMailSpam().
  auto mail = string{argv[1]};
  mail.resize(kMaxMailSize, '\0');
  mail[kMaxMailSize - 1] = '\0';

  // Encrypt the mail on the client side.
  auto mail_ciphertext = FheString::Encrypt(mail, key);
  cout << "[Client] Mail to be checked for spam: \"" << mail << "\"" << endl;
  cout << "[Client] Mail encryption done" << endl << endl;

  // Perform mail spam detection on the server side.
  cout << "\t\t\t\t\t\t\t[Server] Computing..." << endl;
  auto cipher_result = FheInt{params};
  XLS_CHECK_OK(isMailSpam(cipher_result.get(), mail_ciphertext.get(), cloud_key));
  cout << "\t\t\t\t\t\t\t[Server] Computation done" << endl;

  // Decrypt the result on the client side.
  const auto is_spam = cipher_result.Decrypt(key);
  cout << "[Client] Decrypted result: " << is_spam << endl;
  if (is_spam) {
    cout << "[Client] Mail is spam!" << endl;
  } else {
    cout << "[Client] Mail is not spam." << endl;
  }
}
