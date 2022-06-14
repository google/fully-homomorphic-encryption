# Copyright 2022 Google LLC
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

"""List of classes to unwrap in the FHE transpilers and the stuct transpiler."""

# These are the structs defined in primitives.h.  The list here must match the
# set of definitions in primitives.h, to make sure we unwrap them when
# transpiling them.

FHE_PRIMITIVES = [
    "PrimitiveBool",
    "PrimitiveChar",
    "PrimitiveSignedChar",
    "PrimitiveUnsignedChar",
    "PrimitiveSignedShort",
    "PrimitiveUnsignedShort",
    "PrimitiveSignedInt",
    "PrimitiveUnsignedInt",
    "PrimitiveSignedLong",
    "PrimitiveUnsignedLong",
]
