"""Prep raw Kaggle credit card fraud dataset.

This script merges the train and test CSVs, extracts time-based features
(hour, day of week, month), calculates age, and saves the result as a parquet
file matching the schema expected by encode_data.py.
"""

import argparse
import numpy as np
import pandas as pd


def prep_data(train_csv_path, test_csv_path, output_parquet_path):
  print('Loading raw CSVs...')
  train_df = pd.read_csv(train_csv_path)
  test_df = pd.read_csv(test_csv_path)

  print('Merging datasets...')
  df = pd.concat([train_df, test_df], ignore_index=True)

  print('Extracting features...')
  trans_time = pd.to_datetime(df['trans_date_trans_time'])
  dob = pd.to_datetime(df['dob'])

  df['hour'] = trans_time.dt.hour
  df['day_of_week'] = trans_time.dt.dayofweek
  df['month'] = trans_time.dt.month

  # Age calculation
  df['age'] = trans_time.dt.year - dob.dt.year
  has_birthday_occurred = (trans_time.dt.month > dob.dt.month) | (
      (trans_time.dt.month == dob.dt.month) & (trans_time.dt.day >= dob.dt.day)
  )
  df['age'] = df['age'] - (~has_birthday_occurred).astype(int)

  cols_to_keep = [
      'cc_num',
      'merchant',
      'category',
      'amt',
      'first',
      'last',
      'gender',
      'street',
      'city',
      'state',
      'zip',
      'lat',
      'long',
      'city_pop',
      'job',
      'merch_lat',
      'merch_long',
      'is_fraud',
      'hour',
      'day_of_week',
      'month',
      'age',
  ]

  df_prepped = df[cols_to_keep]

  print(f'Saving prepped data to {output_parquet_path}...')
  df_prepped.to_parquet(output_parquet_path, index=False)
  print('Done.')


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Prep raw Kaggle CSVs')
  parser.add_argument(
      '--train_csv', required=True, help='Path to fraudTrain.csv'
  )
  parser.add_argument('--test_csv', required=True, help='Path to fraudTest.csv')
  parser.add_argument(
      '--output',
      default='sparkov_fraud_prepped.parquet',
      help='Path to output parquet',
  )
  args = parser.parse_args()

  prep_data(args.train_csv, args.test_csv, args.output)
