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

# To build and run:
#
#   docker build -t google-fhe-transpiler -f docker/ubuntu-jammy.Dockerfile .
#   docker run --rm -i -t google-fhe-transpiler bash

FROM ubuntu:jammy-20221101

# Install required packages.
RUN apt-get update && apt-get install -y \
  gcc \
  git \
  libtinfo5 \
  python3 \
  python3-pip \
  python-is-python3 \
  autoconf \
  libreadline-dev \
  flex \
  bison \
  wget

# Install deps required to install LLVM
RUN apt-get install -y \
  lsb-release \
  software-properties-common \
  gnupg

RUN wget -O llvm.sh https://apt.llvm.org/llvm.sh \
  && chmod +x llvm.sh \
  # It's critical that this version matches the version specified in
  # llvm_toolchain in the WORKSPACE file.
  && ./llvm.sh 13

# Install bazel
RUN wget -O bazel "https://github.com/bazelbuild/bazel/releases/download/5.3.2/bazel-5.3.2-linux-x86_64" \
  && test "973e213b1e9207ccdd3ea4730c0f92cbef769ec112ac2b84980583220d8db845  bazel" = "$(sha256sum bazel)" \
  && chmod +x bazel \
  && mv bazel /bin/bazel

WORKDIR /usr/src/fhe/

# This step implies docker must be run from the repository root
COPY . .

# Build all targets.
RUN bazel build ...
