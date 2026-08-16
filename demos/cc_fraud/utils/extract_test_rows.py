"""Extracts 10 fraud + 10 non-fraud rows from parquet dataset into a CSV."""

import argparse
import os
import pickle
import sys

import pandas as pd

from demos.common.python import path_utils


def main():
  parser = argparse.ArgumentParser(
      description=(
          "Extract 10 fraud + 10 non-fraud rows from encoded parquet dataset"
          " into CSV."
      )
  )
  parser.add_argument(
      "--input",
      type=str,
      default="demos/cc_fraud/data/sparkov_fraud_encoded.parquet",
      help="Path to input encoded parquet file",
  )
  parser.add_argument(
      "--output",
      type=str,
      default="demos/cc_fraud/data/test_rows.csv",
      help="Path to output CSV file",
  )
  parser.add_argument(
      "--feature_cols",
      type=str,
      default="demos/cc_fraud/data/feature_cols.pkl",
      help="Path to feature columns pickle file",
  )
  parser.add_argument(
      "--num_fraud",
      type=int,
      default=10,
      help="Number of fraud samples to extract",
  )
  parser.add_argument(
      "--num_non_fraud",
      type=int,
      default=10,
      help="Number of non-fraud samples to extract",
  )
  args = parser.parse_args()

  feature_cols_path = path_utils.resolve_path(args.feature_cols)
  encoded_data_path = path_utils.resolve_path(args.input)
  output_csv_path = path_utils.resolve_path(args.output)

  if not os.path.exists(encoded_data_path):
    print(
        f"Error: Encoded data not found at {encoded_data_path}",
        file=sys.stderr,
    )
    sys.exit(1)

  if not os.path.exists(feature_cols_path):
    print(
        f"Error: Feature columns file not found at {feature_cols_path}",
        file=sys.stderr,
    )
    sys.exit(1)

  feature_cols = pickle.load(open(feature_cols_path, "rb"))
  df = pd.read_parquet(encoded_data_path)

  fraud = df[df["is_fraud"] == 1].head(args.num_fraud)
  not_fraud = df[df["is_fraud"] == 0].head(args.num_non_fraud)
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
