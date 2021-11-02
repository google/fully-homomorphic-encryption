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

FROM debian:bullseye-20211011

# Install required packages.
RUN apt-get update && apt-get install -y \
	gcc \
	git \
	libtinfo5 \
	python \
	python3 \
	python3-pip \
	wget

# Install bazel
RUN wget -O bazel "https://github.com/bazelbuild/bazel/releases/download/4.0.0/bazel-4.0.0-linux-x86_64" \
	&& test "7bee349a626281fc8b8d04a7a0b0358492712377400ab12533aeb39c2eb2b901  bazel" = "$(sha256sum bazel)" \
	&& chmod +x bazel \
	&& mv bazel /bin/bazel

WORKDIR /usr/src/fhe/

COPY . .

# Build all targets.
RUN bazel build ...
