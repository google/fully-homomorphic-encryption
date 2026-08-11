"""Export a torch model to mlir linalg."""
import os
import re
import sys

try:
  import torch
  import torch_mlir

  TORCH_MLIR_AVAILABLE = True
except ImportError:
  TORCH_MLIR_AVAILABLE = False


def export_and_postprocess(
    model,
    example_input,
    output_file,
    entrypoint_name,
):
  """Exports a PyTorch model to MLIR and applies standard modifications.

  Args:
    model: The PyTorch model to export.
    example_input: Sample input (or tuple of inputs) for the model.
    output_file: Path to save the MLIR file.
    entrypoint_name: New name for the entrypoint function (e.g., 'mnist',
      'hotword').
  """
  if not TORCH_MLIR_AVAILABLE:
    print(
        "ERROR: torch-mlir is not installed in this environment.",
        file=sys.stderr,
    )
    print(
        "This script is provided as a reference for how the MLIR was"
        " generated.",
        file=sys.stderr,
    )
    sys.exit(1)

  model.eval()
  module = torch_mlir.fx.export_and_import(
      model,
      example_input,
      output_type=torch_mlir.fx.OutputType.LINALG_ON_TENSORS,
  )
  mlir_str = module.operation.get_asm(large_elements_limit=10)

  # Rename @main or @forward to @entrypoint_name
  mlir_str = re.sub(r"@main\b", f"@{entrypoint_name}", mlir_str)
  mlir_str = re.sub(r"@forward\b", f"@{entrypoint_name}", mlir_str)

  # Inject {secret.secret} into %arg0, %arg1, etc. in the entrypoint signature
  def inject_secrets(match):
    sig = match.group(0)
    # Match %argX: tensor<...> and add {secret.secret} if not present
    return re.sub(
        r"(%arg\d+:\s*tensor<[^>]+>)(?!\s*\{secret\.secret\})",
        r"\1 {secret.secret}",
        sig,
    )

  mlir_str = re.sub(
      rf"func\.func @{entrypoint_name}\([^)]*\)",
      inject_secrets,
      mlir_str,
  )

  os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
  with open(output_file, "w") as f:
    f.write(mlir_str)
  print(f"Successfully converted model and saved to '{output_file}'")
