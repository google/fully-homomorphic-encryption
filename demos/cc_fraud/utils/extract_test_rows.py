"""Extracts 10 fraud + 10 non-fraud rows from parquet dataset into a CSV."""

import os
import pickle
import sys

import pandas as pd

from demos.common.python import path_utils


def main():
  feature_cols_path = path_utils.resolve_path(
      "demos/cc_fraud/data/feature_cols.pkl"
  )
  encoded_data_path = path_utils.resolve_path(
      "demos/cc_fraud/data/sparkov_fraud_encoded.parquet"
  )
  output_csv_path = path_utils.resolve_path(
      "demos/cc_fraud/data/test_rows.csv"
  )

  if not os.path.exists(encoded_data_path):
    print(
        f"Error: Encoded data not found at {encoded_data_path}",
        file=sys.stderr,
    )
    sys.exit(1)

  feature_cols = pickle.load(open(feature_cols_path, "rb"))
  df = pd.read_parquet(encoded_data_path)

  fraud = df[df["is_fraud"] == 1].head(10)
  not_fraud = df[df["is_fraud"] == 0].head(10)
  sample = pd.concat([not_fraud, fraud]).reset_index(drop=True)

  out = sample[["is_fraud"] + feature_cols]
  out.to_csv(output_csv_path, index=False, float_format="%.8f")

  print(
      f"Wrote {len(out)} rows ({len(fraud)} fraud, {len(not_fraud)} not-fraud)"
      f" to {output_csv_path}"
  )
  print(f"Columns: is_fraud + {len(feature_cols)} features")


if __name__ == "__main__":
  main()
