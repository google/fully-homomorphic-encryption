"""Evaluate the fraud detection model with debug callbacks using OpenFHE."""

import argparse
import os
import sys
import time

import numpy as np
import pandas as pd

from demos.cc_fraud.openfhe import fraud_model_debug_pybind as fraud_model_pybind
from demos.cc_fraud.utils.data_utils import load_test_row
from demos.common.python import path_utils

resolve_path = path_utils.resolve_path


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      "--row_idx",
      type=int,
      default=0,
      help="Row index from test_rows.csv to evaluate",
  )
  parser.add_argument("--csv_path", type=str, default="test_rows.csv")
  args = parser.parse_args()

  # Set environment variable for the C++ debug helper
  os.environ["HEIR_DEBUG_ROW_IDX"] = str(args.row_idx)

  csv_path = args.csv_path
  if csv_path == "test_rows.csv":
    csv_path = resolve_path(
        "demos/cc_fraud/data/test_rows.csv"
    )

  print(f"Loading test row {args.row_idx} from {csv_path}...")
  t0 = time.time()
  features, expected_label = load_test_row(csv_path, args.row_idx)
  print(f"  Took {time.time() - t0:.4f} seconds")
  print(f"  Expected label (is_fraud): {expected_label}")
  print(f"  Feature vector size: {len(features)}")
  print(f"  First 5 features: {features[:5]}")

  # Initialize crypto context
  print("Generating crypto context...")
  t0 = time.time()
  cc = fraud_model_pybind.cc_fraud__generate_crypto_context()
  print(f"  Took {time.time() - t0:.4f} seconds")

  print("Generating key pair...")
  t0 = time.time()
  key_pair = cc.KeyGen()
  public_key = key_pair.publicKey
  secret_key = key_pair.secretKey
  print(f"  Took {time.time() - t0:.4f} seconds")

  print("Configuring crypto context...")
  t0 = time.time()
  cc = fraud_model_pybind.cc_fraud__configure_crypto_context(cc, secret_key)
  print(f"  Took {time.time() - t0:.4f} seconds")

  # Encrypt input features
  print("Encrypting input features...")
  t0 = time.time()
  encrypted_features = fraud_model_pybind.cc_fraud__encrypt__arg0(
      cc, features, public_key
  )
  print(f"  Took {time.time() - t0:.4f} seconds")

  # Run preprocessing (reused across inferences)
  print("Running preprocessing...")
  t0 = time.time()
  prep_struct = fraud_model_pybind.cc_fraud__preprocessing(cc)
  print(f"  Took {time.time() - t0:.4f} seconds")

  # Call the FHE function (using preprocessed weights and passing secret key
  # for debug)
  print("\n--- Starting FHE Evaluation (with Debug Callbacks) ---")
  t0 = time.time()
  ct_zero_1 = fraud_model_pybind.cc_fraud__encrypt__zero__0(cc, public_key)
  ct_zero_2 = fraud_model_pybind.cc_fraud__encrypt__zero__1(cc, public_key)
  encrypted_output = fraud_model_pybind.cc_fraud__preprocessed(
      cc,
      secret_key,
      encrypted_features,
      ct_zero_1,
      ct_zero_2,
      prep_struct,
  )
  print(f"--- FHE Evaluation Completed in {time.time() - t0:.4f} seconds ---\n")

  # Decrypt output
  print("Decrypting final output...")
  t0 = time.time()
  decrypted_logits = fraud_model_pybind.cc_fraud__decrypt__result0(
      cc, encrypted_output, secret_key
  )
  print(f"  Took {time.time() - t0:.4f} seconds")

  print(f"Decrypted logits: {decrypted_logits}")
  predicted_class = int(np.argmax(decrypted_logits))
  print(f"Predicted class: {predicted_class}")

  if predicted_class == expected_label:
    print("SUCCESS: Predicted class matches expected label!")
  else:
    print("FAILURE: Predicted class does NOT match expected label!")
    sys.exit(1)


if __name__ == "__main__":
  main()
