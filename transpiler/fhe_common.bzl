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

"""Common helper utilities for fhe.bzl."""

def executable_attr(label):
    """A helper for declaring internal executable dependencies."""
    return attr.label(
        default = Label(label),
        allow_single_file = True,
        executable = True,
        cfg = "exec",
    )

def run_with_stem(ctx, stem, inputs, out_ext, tool, args, entry = None):
    """A helper to run a shell script and capture the output.

    Args:
      ctx:  The blaze context.
      stem: Stem for the output file.
      inputs: A list of files used by the shell.
      out_ext: An extension to add to the current label for the output file.
      tool: What tool to run.
      args: A list of arguments to pass to the tool.
      entry: If specified, it points to a file containing the entry point; that
             information is extracted and provided as value to the --top
             command-line switch.

    Returns:
      The File output.
    """
    out = ctx.actions.declare_file("%s%s" % (stem, out_ext))
    arguments = " ".join(args)
    if entry != None:
        arguments += " --top $(cat {})".format(entry.path)
    ctx.actions.run_shell(
        inputs = inputs,
        outputs = [out],
        tools = [tool],
        command = "%s %s > %s" % (tool.path, arguments, out.path),
    )
    return out
