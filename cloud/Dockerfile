# Copyright 2026 Google LLC
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

# Use the lightweight GPU runtime image (no heavy C++ compilers needed)
FROM nvidia/cuda:12.8.2-runtime-ubuntu24.04

# Install wget, dependencies, yosys, Bazel prerequisites, and create the 'abc' symlink
RUN apt-get update && apt-get install -y --no-install-recommends \
    wget \
    ca-certificates \
    libstdc++6 \
    yosys \
    build-essential \
    python3 \
    unzip \
    zip \
    git \
    nano \
    && rm -rf /var/lib/apt/lists/* \
    && ln -s /usr/bin/yosys-abc /usr/local/bin/yosys-abc \
    && ln -s /usr/bin/yosys-abc /usr/local/bin/abc

# Explicitly tell the Yosys optimizer exactly where the ABC binary lives
ENV ABC=/usr/local/bin/abc

WORKDIR /usr/local/bin

# Download Bazelisk (the official Bazel wrapper) and name it 'bazel'
RUN wget -qO /usr/local/bin/bazel https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64 \
    && chmod +x /usr/local/bin/bazel

# Download the pre-built nightly binaries directly from Google's HEIR releases
RUN wget -qO /usr/local/bin/heir-opt https://github.com/google/heir/releases/download/nightly/heir-opt-manylinux_2_28_x86_64 \
    && wget -qO /usr/local/bin/heir-translate https://github.com/google/heir/releases/download/nightly/heir-translate-manylinux_2_28_x86_64 \
    && chmod +x /usr/local/bin/heir-opt /usr/local/bin/heir-translate


# Create a non-root user for Kubernetes security best practices
RUN useradd --create-home --shell /bin/bash heiruser
USER heiruser
WORKDIR /home/heiruser

RUN wget -q https://raw.githubusercontent.com/google/heir/main/tests/Examples/jaxite/add_one_lut3.mlir \
    && wget -q https://raw.githubusercontent.com/google/heir/main/tests/Examples/jaxite/pmap_add_one_lut3.mlir

RUN git clone https://github.com/google/heir.git /tmp/heir-repo \
    && cp -r /tmp/heir-repo/. /home/heiruser/ \
    && rm -rf /tmp/heir-repo

# Set the default execution command for the container
ENTRYPOINT ["/usr/local/bin/heir-opt"]
CMD ["--help"]
