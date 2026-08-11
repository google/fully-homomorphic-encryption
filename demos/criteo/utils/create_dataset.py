import math
import tarfile
from os import makedirs
from os.path import exists, join
from urllib.request import urlretrieve

import numpy as np


url = "https://go.criteo.net/criteo-research-kaggle-display-advertising-challenge-dataset.tar.gz"
files = ["dense.bin", "sparse.bin", "label.bin", "counts.bin"]
raw_dir = "raw"
out_dir = "processed"
chunk_rows = 100_000


def dense_value(raw):
    if raw == "":
        return 0.0
    value = float(raw)
    return math.log1p(value) if value > 0 else 0.0


def sparse_value(raw, mapping):
    token = "0" if raw == "" else raw
    if token not in mapping:
        mapping[token] = len(mapping)
    return mapping[token]


def row_values(line):
    fields = line.rstrip("\r\n").split("\t")
    label = int(fields[0])
    dense = [dense_value(raw) for raw in fields[1:14]]
    sparse = fields[14:]
    return label, dense, sparse


def processed_ready(out_dir):
    return all(exists(join(out_dir, name)) for name in files)


def download_train(raw_dir):
    makedirs(raw_dir, exist_ok=True)
    archive = join(raw_dir, "criteo.tar.gz")
    train = join(raw_dir, "train.txt")

    if not exists(train):
        if not exists(archive):
            urlretrieve(url, archive)
        with tarfile.open(archive) as source:
            source.extract("train.txt", raw_dir)

    return train


def prepare(raw_train, out_dir):
    makedirs(out_dir, exist_ok=True)

    mappings = [{} for _ in range(26)]
    dense_chunk = []
    sparse_chunk = []
    label_chunk = []
    rows = 0

    def write_chunk():
        np.asarray(dense_chunk, dtype=np.float32).tofile(dense_file)
        np.asarray(sparse_chunk, dtype=np.int32).tofile(sparse_file)
        np.asarray(label_chunk, dtype=np.int32).tofile(label_file)
        dense_chunk.clear()
        sparse_chunk.clear()
        label_chunk.clear()

    with open(raw_train, "r", encoding="utf-8", newline="") as source:
        with open(join(out_dir, "dense.bin"), "wb") as dense_file:
            with open(join(out_dir, "sparse.bin"), "wb") as sparse_file:
                with open(join(out_dir, "label.bin"), "wb") as label_file:
                    for line in source:
                        label, dense, sparse_tokens = row_values(line)
                        sparse = [
                            sparse_value(raw, mappings[index])
                            for index, raw in enumerate(sparse_tokens)
                        ]

                        dense_chunk.append(dense)
                        sparse_chunk.append(sparse)
                        label_chunk.append(label)
                        rows += 1

                        if rows % chunk_rows == 0:
                            write_chunk()

                    if rows % chunk_rows:
                        write_chunk()

    sizes = [len(mapping) for mapping in mappings]
    np.asarray(sizes, dtype=np.int32).tofile(join(out_dir, "counts.bin"))

    print(f"prepared {rows} rows in {out_dir}")


def main():
    if processed_ready(out_dir):
        print(f"processed Criteo files already exist in {out_dir}")
        return

    train = download_train(raw_dir)
    prepare(train, out_dir)


if __name__ == "__main__":
    main()
