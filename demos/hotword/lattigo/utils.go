package main

import (
	"archive/zip"
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
)

// skipNpyHeader reads and verifies the NPY magic, then skips the header.
// It returns the reader positioned at the start of the data.
func skipNpyHeader(rc io.Reader) error {
	header := make([]byte, 10)
	if _, err := io.ReadFull(rc, header); err != nil {
		return err
	}

	if !bytes.Equal(header[:6], []byte{0x93, 'N', 'U', 'M', 'P', 'Y'}) {
		return fmt.Errorf("invalid magic")
	}

	headerLen := binary.LittleEndian.Uint16(header[8:10])
	headerDict := make([]byte, headerLen)
	if _, err := io.ReadFull(rc, headerDict); err != nil {
		return err
	}
	return nil
}

func loadTestRow(npzPath string, targetRowIdx int) ([]float32, int, error) {
	r, err := zip.OpenReader(npzPath)
	if err != nil {
		return nil, 0, err
	}
	defer r.Close()

	var label int
	var features []float32

	// Read label first (y.npy)
	for _, f := range r.File {
		if f.Name == "y.npy" {
			rc, err := f.Open()
			if err != nil {
				return nil, 0, err
			}
			defer rc.Close()

			if err := skipNpyHeader(rc); err != nil {
				return nil, 0, fmt.Errorf("error reading y.npy header: %v", err)
			}

			// Seek to targetRowIdx
			// y is 1D array of int64
			var val int64
			// Skip targetRowIdx * 8 bytes
			if _, err := io.CopyN(io.Discard, rc, int64(targetRowIdx)*8); err != nil {
				return nil, 0, fmt.Errorf("error seeking in y.npy: %v", err)
			}
			if err := binary.Read(rc, binary.LittleEndian, &val); err != nil {
				return nil, 0, fmt.Errorf("error reading label: %v", err)
			}
			label = int(val)
			break
		}
	}

	// Read features (x.npy)
	for _, f := range r.File {
		if f.Name == "x.npy" {
			rc, err := f.Open()
			if err != nil {
				return nil, 0, err
			}
			defer rc.Close()

			if err := skipNpyHeader(rc); err != nil {
				return nil, 0, fmt.Errorf("error reading x.npy header: %v", err)
			}

			// x has shape (1000, 40, 101) of float32 (4 bytes)
			// We want to skip targetRowIdx samples.
			// Each sample is 40 * 101 * 4 bytes = 16160 bytes.
			skipBytes := int64(targetRowIdx) * 40 * 101 * 4
			if _, err := io.CopyN(io.Discard, rc, skipBytes); err != nil {
				return nil, 0, fmt.Errorf("error seeking in x.npy: %v", err)
			}

			// Now read one sample of shape (40, 101) and slice to (40, 98)
			features = make([]float32, 0, 40*98)
			for row := 0; row < 40; row++ {
				// Read 98 floats
				for col := 0; col < 98; col++ {
					var val float32
					if err := binary.Read(rc, binary.LittleEndian, &val); err != nil {
						return nil, 0, fmt.Errorf("error reading feature at row %d col %d: %v", row, col, err)
					}
					features = append(features, val)
				}
				// Discard remaining 3 floats (101 - 98)
				if _, err := io.CopyN(io.Discard, rc, 3*4); err != nil {
					return nil, 0, fmt.Errorf("error discarding columns at row %d: %v", row, err)
				}
			}
			break
		}
	}

	return features, label, nil
}

func loadAllTestRows(npzPath string) ([][]float32, []int, error) {
	r, err := zip.OpenReader(npzPath)
	if err != nil {
		return nil, nil, err
	}
	defer r.Close()

	var labels []int
	var allFeatures [][]float32

	// Read labels (y.npy)
	for _, f := range r.File {
		if f.Name == "y.npy" {
			rc, err := f.Open()
			if err != nil {
				return nil, nil, err
			}
			defer rc.Close()

			if err := skipNpyHeader(rc); err != nil {
				return nil, nil, fmt.Errorf("error reading y.npy header: %v", err)
			}

			// Read 1000 int64s
			labels = make([]int, 1000)
			for i := 0; i < 1000; i++ {
				var val int64
				if err := binary.Read(rc, binary.LittleEndian, &val); err != nil {
					if err == io.EOF && i > 0 {
						labels = labels[:i]
						break
					}
					return nil, nil, fmt.Errorf("error reading label %d: %v", i, err)
				}
				labels[i] = int(val)
			}
			break
		}
	}

	// Read features (x.npy)
	for _, f := range r.File {
		if f.Name == "x.npy" {
			rc, err := f.Open()
			if err != nil {
				return nil, nil, err
			}
			defer rc.Close()

			if err := skipNpyHeader(rc); err != nil {
				return nil, nil, fmt.Errorf("error reading x.npy header: %v", err)
			}

			numSamples := len(labels)
			allFeatures = make([][]float32, numSamples)

			for s := 0; s < numSamples; s++ {
				features := make([]float32, 0, 40*98)
				for row := 0; row < 40; row++ {
					for col := 0; col < 98; col++ {
						var val float32
						if err := binary.Read(rc, binary.LittleEndian, &val); err != nil {
							return nil, nil, fmt.Errorf("error reading feature at sample %d row %d col %d: %v", s, row, col, err)
						}
						features = append(features, val)
					}
					// Discard remaining 3 floats
					if _, err := io.CopyN(io.Discard, rc, 3*4); err != nil {
						return nil, nil, fmt.Errorf("error discarding columns at sample %d row %d: %v", s, row, err)
					}
				}
				allFeatures[s] = features
			}
			break
		}
	}

	return allFeatures, labels, nil
}
