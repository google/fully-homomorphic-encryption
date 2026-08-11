"""Evaluate a suite of test rows using OpenFHE."""

import argparse
import os
import time

import numpy as np
import pandas as pd

from demos.cc_fraud.openfhe import fraud_model_pybind
from demos.cc_fraud.utils.data_utils import load_all_test_rows
from demos.common.python import path_utils

resolve_path = path_utils.resolve_path


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("--csv_path", type=str, default="test_rows.csv")
  parser.add_argument(
      "--limit", type=int, default=None, help="Limit number of rows to test"
  )
  args = parser.parse_args()

  csv_path = args.csv_path
  if csv_path == "test_rows.csv":
    csv_path = resolve_path(
        "demos/cc_fraud/data/test_rows.csv"
    )

  print(f"Loading all test rows from {csv_path}...")
  t0 = time.time()
  all_features, expected_labels = load_all_test_rows(csv_path)
  if args.limit is not None:
    all_features = all_features[: args.limit]
    expected_labels = expected_labels[: args.limit]
  num_rows = len(all_features)
  print(f"  Loaded {num_rows} rows in {time.time() - t0:.4f} seconds")

  # Initialize crypto context (ONCE)
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

  # Run preprocessing (ONCE)
  print("Running preprocessing for model weights...")
  t0 = time.time()
  prep_struct = fraud_model_pybind.cc_fraud__preprocessing(cc)
  print(f"  Took {time.time() - t0:.4f} seconds")

  print("\nStarting FHE evaluation suite...")
  correct_count = 0
  misclassifications = []

  suite_start_time = time.time()
  for idx in range(num_rows):
    features = all_features[idx]
    expected_label = expected_labels[idx]

    # Encrypt input features and zero accumulators
    encrypted_features = fraud_model_pybind.cc_fraud__encrypt__arg0(
        cc, features, public_key
    )
    ct_zero_1 = fraud_model_pybind.cc_fraud__encrypt__zero__0(cc, public_key)
    ct_zero_2 = fraud_model_pybind.cc_fraud__encrypt__zero__1(cc, public_key)

    # Call the FHE function (using preprocessed weights)
    encrypted_output = fraud_model_pybind.cc_fraud__preprocessed(
        cc,
        encrypted_features,
        ct_zero_1,
        ct_zero_2,
        prep_struct,
    )

    # Decrypt output
    decrypted_logits = fraud_model_pybind.cc_fraud__decrypt__result0(
        cc, encrypted_output, secret_key
    )
    predicted_class = int(np.argmax(decrypted_logits))

    is_correct = predicted_class == expected_label
    status = "SUCCESS" if is_correct else "MISCLASSIFIED"

    print(
        f"Row {idx:3d}: expected {expected_label}, got {predicted_class}"
        f" ({status})"
    )

    if is_correct:
      correct_count += 1
    else:
      misclassifications.append((idx, expected_label, predicted_class))

  total_time = time.time() - suite_start_time
  accuracy = correct_count / num_rows if num_rows > 0 else 0
  print(
      f"\nSuite completed in {total_time:.2f} seconds (average"
      f" {total_time/num_rows:.2f}s per row)"
  )
  print(f"Accuracy: {correct_count}/{num_rows} ({accuracy:.2%})")

  if misclassifications:
    print("\nSummary of Misclassifications:")
    for idx, exp, pred in misclassifications:
      print(f"  Row {idx:3d}: expected {exp}, got {pred}")
  else:
    print("\nNO MISCLASSIFICATIONS!")


if __name__ == "__main__":
  main()
