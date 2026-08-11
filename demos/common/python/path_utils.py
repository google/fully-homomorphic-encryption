import os
from python.runfiles import runfiles


def resolve_path(path):
  """Resolves a path using runfiles, with fallback to filesystem."""
  try:
    r = runfiles.Create()
    if r is not None:
      resolved = r.Rlocation(path)
      if resolved and os.path.exists(resolved):
        return resolved
  except Exception:  # pylint: disable=broad-except
    pass

  if os.path.exists(path):
    return path
  stripped = path.split("/", 1)[1] if path.startswith("fully_homomorphic_encryption/") else path
  if os.path.exists(stripped):
    return stripped
  return path
