# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Rules for generating FHE-C++."""

load(
    "//transpiler:fhe.bzl",
    "fhe_cc_library",
)

CRYPTOSYSTEM_DEPS = {
    "tfhe": [
        "//transpiler/data:tfhe_data",
        "@tfhe//:libtfhe",
    ],
    "openfhe": [
        "//transpiler/data:openfhe_data",
        "@openfhe//:binfhe",
    ],
    "cleartext": [
        "//transpiler/data:cleartext_data",
    ],
}

def gen_fhe_cc_library_for_all_optimizers_translators_cryptosystems(
        name,
        src,
        hdrs,
        num_opt_passes = 1,
        **kwargs):
    """A rule for building all supported FHE-based cc_libraries.

    These libraries generated include ones across cryptosystems(tfhe, openfhe,
      cleartext), optimizers(xls, yosys) and translators(transpiled, interpreted)

    Example usage:
        gen_fhe_cc_library_for_all_optimizers_translators_cryptosystems(
            name = "secret_computation",
            src = "secret_computation.cc",
            hdrs = ["secret_computation.h"],
            num_opt_passes = 2,
        )
        cc_binary(
            name = "secret_computation_consumer",
            srcs = ["main.cc"],
            deps = [":secret_computation_tfhe_xls_transpiled"],
        )

    To generate just the transpiled sources, you can build the "<TARGET>.transpiled_files"
    subtarget; in the above example, you could run:
        blaze build :secret_computation_tfhe_xls_transpiled.transpiled_files

    Args:
      name: The name of the cc_library target.
      src: The transpiler-compatible C++ file that are processed to create the library.
      hdrs: The list of header files required while transpiling the `src`.
      num_opt_passes: The number of optimization passes to run on XLS IR (default 1).
            Values <= 0 will skip optimization altogether.
            (Only affects the XLS optimizer.)
      **kwargs: Keyword arguments to pass through to the cc_library target.
    """

    for encryption in ["tfhe", "openfhe", "cleartext"]:
        for optimizer in ["xls", "yosys"]:
            for translator in ["transpiled", "interpreted"]:
                # Yosys transpiler is not defined for any encryption scheme.
                if (optimizer == "yosys" and translator == "transpiled"):
                    continue

                # cleartext interpreter is not defined for xls
                if (encryption == "cleartext" and optimizer == "xls" and translator == "interpreted"):
                    continue

                fhe_cc_library(
                    name = "{}_{}_{}_{}".format(name, encryption, optimizer, translator),
                    src = src,
                    hdrs = hdrs,
                    num_opt_passes = num_opt_passes,
                    encryption = encryption,
                    optimizer = optimizer,
                    interpreter = translator == "interpreted",
                    **kwargs
                )

def gen_demos(
        name,
        encryption,
        srcs,
        deps):
    """A rule for generating binaries for the specified cryptosystem backend.

    These libraries generated include ones across optimizers(xls, yosys) and
    translators(transpiled, interpreted)

    Example usage:
        gen_demos(
            name = "secret_computation_consumer",
            encryption = "tfhe"
            src = ""main.cc",
            deps = [":secret_computation_plaintext"]
        )

    Args:
      name: The name of the cc_library target.
      encryption: the backend cryptosystem used i.e. tfhe, openfhe or cleartext
      srcs: The transpiler-compatible C++ file that are processed to create the library.
      deps: The list of header files required while transpiling the `src`.
    """
    for optimizer in ["xls", "yosys"]:
        for translator in ["transpiled", "interpreted"]:
            # Yosys transpiler is not defined for any encryption scheme.
            if (optimizer == "yosys" and translator == "transpiled"):
                continue

            # cleartext interpreter is not defined for xls.
            if (encryption == "cleartext" and optimizer == "xls" and translator == "interpreted"):
                continue

            calculated_copts = []
            if translator == "interpreted":
                calculated_copts.append("-DUSE_{}INTERPRETED_{}".format("YOSYS_" if optimizer == "yosys" else "", encryption.upper()))

            native.cc_binary(
                name = "{}_{}_{}_testbench".format(name, optimizer, translator),
                srcs = srcs,
                deps = deps + CRYPTOSYSTEM_DEPS[encryption] + [
                    ":{}_{}_{}".format(name, optimizer, translator),
                ],
                copts = calculated_copts,
            )
