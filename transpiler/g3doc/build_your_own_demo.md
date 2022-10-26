# How to build your own demo

This walkthrough will help you generate FHE-C++ transpiled from your own C++
code. These instructions assume that all dependencies have already been
installed (as described in the [`README`](../README.md)).

## 1. Write your C++ code

To begin, navigate to the [`transpiler/codelab`](../codelab) folder, create a
directory, and put your own C++ program containing the code you will want
transpiled in said folder. You must ensure the following is included in your
code:

*   The function where the execution should begin must be marked with `#pragma
    hls_top`, so the transpiler knows where to begin transpiling. More
    specifically, the transpiler will create transpiled implementations of the
    function marked with `#pragma hls_top`, and all function calls from within
    said marked function.
*   All loops must be marked with `#pragma hls_unroll yes` to inform XLS[cc] to
    unroll the loop, which is required by the transpiling process.
*   There are limitations of what C++ code can be transpiled. See the
    [`README`](../README.md) for the current limitations and possible
    workarounds.

To illustrate using the `string_cap_char` example, examine the code in the
[`string_cap_char.cc`](../codelab/string_cap_char/string_cap_char.cc) file (shown
below), which capitalizes the first letter of every word in a given input
string:

```cpp
State::State() : last_was_space_(true) {}

unsigned char State::process(unsigned char c) {
  unsigned char ret = c;

  if (last_was_space_ && (c >= 'a') && (c <= 'z')) ret -= ('a' - 'A');

  last_was_space_ = (c == ' ');
  return ret;
}

#pragma hls_top
char my_package(State &st, char c) { return st.process(c); }
```

Note the following about the code above: * The C++ program's entry point must be
marked with `#pragma hls_top`. In the case of `string_cap_char`, this is the
`my_package` function. * This example is written such that the function above is
called for each character. However, the
[`string_cap/string_cap.cc`](../examples/string_cap/string_cap.cc) example
illustrates how the same result can be achieved by instead inputting the entire
string and internally iterating on each character. * You may notice that
`string_cap.cc` and other code examples may have a corresponding `.h` header
file. These are optional; header files are not a requirement.

## 2. Create a `BUILD` file

A `BUILD` file must exist in the directory containing your code for it to be
transpiled. The `BUILD` file should contain the following:

```
load("//transpiler:fhe.bzl", "fhe_cc_library")

fhe_cc_library(
    name = "fn_tfhe",
    src = "fn.cc",
    hdrs = ["fn.h"],
)
```

`fhe_cc_library` creates a Bazel macro that invokes build rules.
<sup>[1](#fhe-cc-library)</sup> The arguments required by `fhe_cc_library` are
as follows:

*   `name` is the name of the package being transpiled. This can be whatever you
    choose.
*   `src` is the C++ file containing the code to be transpiled (discussed in the
    previous section).
*   `hdrs` are any headers required for transpiling `src`. As previously
    mentioned, header files (and thus, this field) are optional.

## 3. Build and transpile your C++ code

The `fhe_cc_library` macro previously discussed generates a `.transpiled_files`
rule that can be directly built to generate intermediate representations of the
code used in the transpiling process. To build and transpile your C++, run a
command such as the following, which assumes the directory containing the code
and `BUILD` file is named "transpiler/codelab/fn", and the `name` chosen above
is "fn_tfhe":

```shell
bazel build -c opt //transpiler/codelab/fn:fn_tfhe.transpiled_files
```

The transpiling process will generate several files, which should be listed in
the above command's output. Feel free to examine these files to better
understand more about the transpiling process. 
The intermediate representation(IR) files can be visualized with ['XLS IR visualization tools'](https://google.github.io/xls/ir_visualization/)

In particular, open the header file bearing the `name` chosen above (e.g.,
`fn_tfhe.h`), and find the signature (i.e., return value and arguments) of the
transpiled function. This function is what needs to be called to perform the
FHE-C++ operations equivalent to the operations in your original C++ code. The
function signature of the transpiled function should be somewhat different than
the function signature in your original C++ code. In particular, there should be
additional arguments needed for performing FHE operations.

## 4. Write your FHE-C++ testbench

You will also need to write an executable C++ testbench that will simulate a
"client" that provides input to and receive output from the transpiled FHE-C++
code on a "server". To illustrate using the `string_cap_char` example, examine
the code in the
[`string_cap_char_tfhe_testbench.cc`](../codelab/string_cap_char/string_cap_char_tfhe_testbench.cc)
file. Below is a simplified version of the code:

```cpp
void TfheStringCap(TfheArray<char>& cipherresult, TfheArray<char>& ciphertext, int data_size,
                  TfheState& cipherstate, const TFheGateBootstrappingCloudKeySet* bk) {

  for (int i = 0; i < data_size; i++) {
    CHECK_OK(my_package(cipherresult[i], cipherstate.get(), ciphertext[i], bk));
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: string_cap_fhe_testbench string_input\n\n");
    return 1;
  }

  const char* input = argv[1];
  // generate a keyset
  TFHEParameters params(120);

  // generate a random key
  std::array<uint32_t, 3> seed = {314, 1592, 657};
  TFHESecretKeySet key(params, seed);

  std::string plaintext(input);
  size_t data_size = plaintext.size();
  std::cout << "plaintext(" << data_size << "):" << plaintext << std::endl;

  // Encrypt data
  auto ciphertext = TfheArray<char>::Encrypt(plaintext, key);
  std::cout << "Encryption done" << std::endl;

  std::cout << "Initial state check by decryption: " << std::endl;
  std::cout << ciphertext.Decrypt(key) << "\n";
  std::cout << "\n";

  State st;
  TfheState cipherstate(params.get());
  cipherstate.SetEncrypted(st, key.get());

  std::cout << "\t\t\t\t\tServer side computation:" << std::endl;
  // Perform string capitalization
  TfheArray<char> cipher_result = {data_size, params};
  TfheStringCap(cipher_result, ciphertext, data_size, cipherstate, key.cloud());
  std::cout << "\t\t\t\t\tComputation done" << std::endl;

  std::cout << "Decrypted result: ";
  std::cout << cipher_result.Decrypt(key) << "\n";
  std::cout << "Decryption done" << std::endl;
}
```

Stepping through the code as it is executed (i.e., starting from `int main`), we
can see the following:

1.  The data to be transformed is input from the command-line (via `argc` &
    `argv`), as is typical for C++ programs. Thus, your testbench may do the
    same, or possibly input data by any means supported in C++.
2.  `const char* input = argv[1];` stores in the input character array from the
    command-line in the variable `input`.
3.  `TFHEParameters params(120);` sets the TFHE security parameters, where 120
    ensures TFHE will provide 128 bits of security. For more information, see
    the [TFHE library](https://tfhe.github.io/tfhe/).
4.  Since this is merely a testbench example, the code uses a hardcoded private
    key for the encryption by setting `std::array<uint32_t, 3> seed = {314,
    1592, 657}; TFHESecretKeySet key(params, seed);`.
5.  By convention, the input string is stored to a variable named `plaintext`
    (i.e., `std::string plaintext(input);`) so it is clear the contents are
    unencrypted.
6.  Get the length of the input string with `size_t data_size =
    plaintext.size();`, to know when there are no more characters to read from
    `plaintext`.
7.  `auto ciphertext = TfheArray<char>::Encrypt(plaintext, key);` encrypts the
    plaintext data with the symmetric secret `key`, and stores the ciphertext in
    an TfheArray<char> variable named `ciphertext`.
8.  To ensure the encryption and decryption work correctly, we call and output
    the result of `ciphertext.Decrypt(key)`, which should match the content of
    `plaintext`.
9.  `TfheArray<char> cipher_result = {data_size, params};` declares the variable
    `cipher_result` that will store the result of the transformation of the
    encrypted data.
10. `TfheStringCap(cipher_result, ciphertext, data_size, state, key.cloud());` calls the
    `TfheStringCap` helper function defined above `int main`. `cipher_result` is
    passed as an output argument. `key.cloud()` is a special key that allows
    operations to be performed on the ciphertext without it being decryptable.
11. `TfheState state(params);` is the FHE representation of the original C++
    code's `State` that holds the `last_was_space_` variable.
12. The code then iterates over each FHE-character in `ciphertext`, sending it
    to the "server" to be transformed, and receiving the resulting FHE-character
    into the `cipherresult` variable.
13. Finally, the client decrypts `cipher_result` to reveal the server's
    transformation, which in this case should be the capitalization of each word
    in the input sentence.

To summarise, the following is the minimum needed to implement a testbench:

1.  The testbench calls the FHE-C++ code using the method signature found in the
    header file generated by the transpiling process.
2.  The testbench must be an executable program (most likely written in C++).
3.  `TFHEParameters params(120);` initializes the TFHE library and specifies the
    security strength of the encryption.
4.  The `TFHESecretKeySet` encryption key is created with the aforementioned
    `TFHEParameters` and a `std::array<uint32_t, 3>` (i.e., an array of 3
    unsigned 32-bit integers).
5.  The encrypted input and output data must be stored in a TFHE-supported data
    type. These are defined in [transpiler/data/tfhe_data.h](../data/tfhe_data.h).
6.  The special key that allows for operations on the encrypted data is obtained
    by calling `TFHESecretKeySet::cloud()` on the previously-generated key.

Once the testbench has been written (e.g., `fn_tfhe_testbench`), it can be
compiled and run a command such as:

```shell
bazel run -c opt fn:fn_tfhe_testbench -- "arg1" "arg2"
```

Note that any arguments to be passed to the testbench are added after two dashes
(i.e., `--`).

## Endnotes

<a name="fhe-cc-library">1</a>: For more information on `fhe_cc_library` and
what parameters can be provided, see the `fhe_cc_library` definition in
[`fhe.bzl`](../fhe.bzl).
