"""Encode raw Sparkov fraud data into the model-ready feature representation."""

import json
import os
import pickle
import sys

import numpy as np
import pandas as pd

from demos.common.python import path_utils


def encode_dataframe(
    df: pd.DataFrame,
    mappings_path: str = None,
    scaler_path: str = None,
    feature_cols_path: str = None,
    label_col: str = "is_fraud",
) -> pd.DataFrame:
  """Encode and scale a raw DataFrame into model-ready features."""
  if mappings_path is None:
    mappings_path = path_utils.resolve_path(
        "demos/cc_fraud/data/encoder_mappings.json"
    )
  if scaler_path is None:
    scaler_path = path_utils.resolve_path(
        "demos/cc_fraud/data/scaler.pkl"
    )
  if feature_cols_path is None:
    feature_cols_path = path_utils.resolve_path(
        "demos/cc_fraud/data/feature_cols.pkl"
    )

  with open(mappings_path) as f:
    mappings = json.load(f)
  scaler = pickle.load(open(scaler_path, "rb"))
  feature_cols = pickle.load(open(feature_cols_path, "rb"))

  out = df.copy()

  # 1. Ordinal encoding
  for col, mapping in mappings["ordinal"].items():
    # Map string keys back to original values (handles int columns like cc_num)
    type_safe_map = {}
    for k, v in mapping.items():
      try:
        # Try preserving the original dtype (handles cc_num as int)
        type_safe_map[type(out[col].iloc[0])(k)] = v
      except Exception:
        type_safe_map[k] = v
    # Default to -1 for unseen categories
    out[col] = out[col].map(type_safe_map).fillna(-1).astype(np.int64)

  # 2. Frequency encoding (none in this dataset, but supported)
  for col, mapping in mappings["frequency"].items():
    type_safe_map = {}
    for k, v in mapping.items():
      try:
        type_safe_map[type(out[col].iloc[0])(k)] = v
      except Exception:
        type_safe_map[k] = v
    out[col] = out[col].map(type_safe_map).fillna(0).astype(np.int64)

  # 3. One-hot encoding (with drop_first=True to match training)
  drop_first = mappings.get("drop_first", True)
  ohe_cols = list(mappings["ohe"].keys())
  new_columns = {}
  for col in ohe_cols:
    categories = sorted(mappings["ohe"][col])
    cats_to_encode = categories[1:] if drop_first else categories
    col_values = out[col].astype(str).values
    for cat in cats_to_encode:
      new_columns[f"{col}_{cat}"] = (col_values == cat).astype(np.uint8)

  out = out.drop(columns=ohe_cols)
  if new_columns:
    ohe_df = pd.DataFrame(new_columns, index=out.index)
    out = pd.concat([out, ohe_df], axis=1)

  # 4. Ensure column order matches what the models expect
  label_series = out[label_col] if label_col in out.columns else None

  # 5. Standardize
  raw_features = out[feature_cols].astype(np.float64).values
  scaled = scaler.transform(raw_features)
  feature_df = pd.DataFrame(scaled, columns=feature_cols, index=out.index)

  if label_series is not None:
    feature_df[label_col] = label_series.values
  return feature_df


def main():
  prepped_path = path_utils.resolve_path(
      "demos/cc_fraud/data/sparkov_fraud_prepped.parquet"
  )
  encoded_path = path_utils.resolve_path(
      "demos/cc_fraud/data/sparkov_fraud_encoded.parquet"
  )

  if not os.path.exists(prepped_path):
    print(f"Error: Prepped data not found at {prepped_path}", file=sys.stderr)
    sys.exit(1)

  print("Loading raw dataset...")
  raw = pd.read_parquet(prepped_path)
  print(f"  Raw shape: {raw.shape}")

  print("Encoding...")
  encoded = encode_dataframe(raw)
  print(f"  Encoded shape: {encoded.shape}")

  # Verify against the supplied encoded reference if it exists
  print()
  ref_encoded_path = path_utils.resolve_path(
      "demos/cc_fraud/data/sparkov_fraud_encoded.parquet"
  )
  if os.path.exists(ref_encoded_path) and ref_encoded_path != encoded_path:
    print(f"Verifying against reference at {ref_encoded_path}...")
    reference = pd.read_parquet(ref_encoded_path)
    if (
        set(encoded.columns) == set(reference.columns)
        and encoded.shape == reference.shape
    ):
      # Compare values within numerical tolerance
      feature_cols = [c for c in encoded.columns if c != "is_fraud"]
      diff = encoded[feature_cols].values - reference[feature_cols].values
      max_abs = float(np.abs(diff).max())
      print(f"  Shapes match: {encoded.shape}")
      print(f"  Max absolute difference vs reference: {max_abs:.2e}")
      if max_abs < 1e-3:
        print(
            "  PASS — encoded output matches the supplied reference (within"
            " float precision)."
        )
      else:
        print("  WARNING — encoded output deviates from supplied reference.")
    else:
      print("  Shape or column mismatch with reference.")

  # Save the encoded dataset
  print(f"Saving encoded dataset to {encoded_path}...")
  encoded.to_parquet(encoded_path, index=False)


if __name__ == "__main__":
  main()
