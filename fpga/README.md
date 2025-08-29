# FHE on FPGA

This document provides a brief introduction to running FHE programs on FPGA. Research on FPGAs is partially funded by a research gift of Google to KU Leuven - COSIC, ERC advanced grant Belfort 101020005 and Belfort Labs BV.

## One time setup
Follow instructions [here](https://github.com/belfortlabs/hello-fpga?tab=readme-ov-file#setup-your-aws-account) to setup an account with Belfort FPGA accelerator VM


## Running FPGA Demos
Follow instructions [here](https://github.com/belfortlabs/hello-fpga?tab=readme-ov-file#prepare-execution-environment) to ssh into the FPGA machine.

### Fuzzy String matching
Leuvenshtein is a fuzzy matching algorithm. Fuzzy matching is a technology to compare two strings while allowing a limited amount of mistakes. For example, it matches "Bilba Biggins" to "Bilbo Baggins". Leuvenshtein is a hand-crafted version of [Levenshtein](https://en.wikipedia.org/wiki/Levenshtein_distance) optimising its implementation for the FHE domain, enabling comparison of encrypted strings. This can be useful for applications, such as banking, government, health care, where data sensitivity is high. For further details, please check out the [publication](https://eprint.iacr.org/2025/012).

To try this, follow instructions [here](https://github.com/belfortlabs/hello-fpga/tree/f2/demos/leuvenshtein)