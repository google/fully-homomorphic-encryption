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

FROM debian:bullseye

# Install required packages.
RUN apt-get update && apt-get install -y \
	gcc \
	git \
	libtinfo5 \
	python \
	python3 \
	python3-pip \
	wget

# Install bazelisk.
RUN wget -O bazelisk "https://github.com/bazelbuild/bazelisk/releases/download/v1.8.0/bazelisk-linux-amd64" \
	&& test "cde5b2769d0633bb4fdc3e41800a1b2c867b68bd724b1dd23258e0ae0ac86cd6  bazelisk" = "$(sha256sum bazelisk)" \
	&& chmod +x bazelisk \
	&& mv bazelisk /bin/bazelisk

WORKDIR /usr/src/fhe/

# Let bazelisk fetch bazel.
COPY .bazelversion .
RUN bazelisk version

COPY . .

# Build all targets.
RUN bazelisk build ...
