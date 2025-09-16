# FHE on FPGA

This document provides a brief introduction to running FHE programs on FPGA. Research on FPGAs is partially funded by a research gift of Google to KU Leuven - COSIC, ERC advanced grant Belfort 101020005 and Belfort Labs BV.

## One time setup
Follow instructions [here](https://github.com/belfortlabs/hello-fpga?tab=readme-ov-file#launch-an-f2-instance) to launch an AWS instance with Belfort FPGA accelerator.


## Running FPGA Demos

SSH into the FPGA machine

```sh
ssh -i <id.pem> ubuntu@<instance_public_dns>
```

Clone the following repository

```sh
git clone https://github.com/belfortlabs/hello-fpga.git
```

Prepare the environment

```sh
cd hello-fpga && ./scripts/prepare_env.sh
```


### Run example program
You can run both CPU and FPGA version of the application and compare the execution time differences.

```sh
cargo run --release --package example --bin weighted-sum
```

```sh
cargo run --release --package example --bin weighted-sum --features fpga
```
You should see the result of the weighted-sum complete much faster with the FPGA feature! In case you run into any issues, please open an issue [here](https://github.com/belfortlabs/hello-fpga/tree/f2).

### Fuzzy String matching
Leuvenshtein is a fuzzy matching algorithm. Fuzzy matching is a technology to compare two strings while allowing a limited amount of mistakes. For example, it matches "Bilba Biggins" to "Bilbo Baggins". Leuvenshtein is a hand-crafted version of [Levenshtein](https://en.wikipedia.org/wiki/Levenshtein_distance) optimising its implementation for the FHE domain, enabling comparison of encrypted strings. This can be useful for applications, such as banking, government, health care, where data sensitivity is high. For further details, please check out the [publication](https://eprint.iacr.org/2025/012).

The demo application showcases the efficient processing of encrypted searches on the Belfort FPGAs. The demo implements following aspects:

- Encrypted query search in an encrypted database of names
- Plaintext query search in an encrypted database using preprocessing techniques
- Execution of above situations, both on CPU and FPGA

```sh
cargo run --release --package leuvenshtein --bin demo --features fpga
```
For more details see [here](https://github.com/belfortlabs/hello-fpga/tree/f2/demos/leuvenshtein)

### AES Transciphering
Transciphering makes FHE compatible with existing systems and tackles the ciphertext
blowup issue. This technique involves encrypting data with standard stream ciphers
like AES, while the AES key itself is encrypted using FHE. Subsequently, a single
homomorphic AES decryption operation can be performed before further computations
on the encrypted data.

This approach facilitates easy FHE encryption compatibility with standard protocols
that utilize AES, such as HTTPS. Furthermore, the ciphertext size remains the same
as the data size with AES, significantly reducing the substantial network
bandwidth and storage requirements associated with conventional FHE encryption
schemes.

An interactive demo for tranciphering with AES cipher on FPGA is available to try.

```sh
cargo run --release --package fhe-aes --bin demo --features fpga
```

For more details see [here](https://github.com/belfortlabs/hello-fpga/blob/f2/demos/fhe-aes)
