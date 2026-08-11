import argparse
import sys
import pandas as pd
import pyarrow


def main():
  parser = argparse.ArgumentParser(
      description="Convert Criteo Parquet file to raw TSV format."
  )
  parser.add_argument("--input", required=True, help="Input parquet file path")
  parser.add_argument("--output", required=True, help="Output TSV file path")
  args = parser.parse_args()

  try:
    df = pd.read_parquet(args.input, engine="pyarrow")
    dense_cols = [f"integer_feature_{i}" for i in range(1, 14)]
    sparse_cols = [f"categorical_feature_{i}" for i in range(1, 27)]
    cols = ["label"] + dense_cols + sparse_cols

    # Cast dense columns to nullable Int64 to avoid float formatting (.0) in TSV
    for col in dense_cols:
      if col in df.columns:
        df[col] = df[col].astype("Int64")

    # Save to TSV without header and index, using empty string for NaNs
    df[cols].to_csv(args.output, sep="\t", header=False, index=False, na_rep="")
    print(f"Successfully converted {args.input} to {args.output}")
  except Exception as e:
    print(f"Error during conversion: {e}", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
  main()
