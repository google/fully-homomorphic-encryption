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
#   docker build -t google-fhe-transpiler -f docker/debian-bullseye.Dockerfile .
#   docker run --rm -i -t google-fhe-transpiler bash

FROM debian:bullseye-20220527

# Install required packages.
RUN apt-get update && apt-get install -y \
  gcc \
  git \
  libtinfo5 \
  python \
  python3 \
  python3-pip \
  autoconf \
  libreadline-dev \
  flex \
  bison \
  wget

# Install bazelisk as bazel
RUN wget -O bazel "https://github.com/bazelbuild/bazelisk/releases/download/v1.12.0/bazelisk-linux-amd64" \
  && test "6b0bcb2ea15bca16fffabe6fda75803440375354c085480fe361d2cbf32501db  bazel" = "$(sha256sum bazel)" \
  && chmod +x bazel \
  && mv bazel /bin/bazel

WORKDIR /usr/src/fhe/

# This step implies docker must be run from the repository root
COPY . .

# Build LLVM. In the case that the above install of LLVM fails, this step will
# result in building LLVM from source, which will take a long time. But if
# it's necessary for some reason, this will allow docker to cache the resulting
# image and build failures unrelated to LLVM will not incur a rebuild of LLVM.
RUN bazel build @llvm_toolchain//:all

# Build all targets.
RUN bazel build ...
