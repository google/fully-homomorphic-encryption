// Package mnist_data provides utilities for loading MNIST dataset.
package mnist_data

import (
	"archive/zip"
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"regexp"
	"strconv"
	"strings"
)

// parseNPY parses a .npy file from a reader and returns its shape, dtype, and raw data.
func parseNPY(r io.Reader) (shape []int, dtype string, data []byte, err error) {
	// Magic header: 6 bytes
	magic := make([]byte, 6)
	if _, err := io.ReadFull(r, magic); err != nil {
		return nil, "", nil, err
	}
	if !bytes.Equal(magic, []byte{0x93, 'N', 'U', 'M', 'P', 'Y'}) {
		return nil, "", nil, errors.New("invalid NPY magic")
	}

	// Version: 2 bytes
	version := make([]byte, 2)
	if _, err := io.ReadFull(r, version); err != nil {
		return nil, "", nil, err
	}

	// Header length: 2 bytes (little endian)
	var headerLen uint16
	if err := binary.Read(r, binary.LittleEndian, &headerLen); err != nil {
		return nil, "", nil, err
	}

	// Header: headerLen bytes
	headerBytes := make([]byte, headerLen)
	if _, err := io.ReadFull(r, headerBytes); err != nil {
		return nil, "", nil, err
	}
	header := string(headerBytes)

	// Parse dtype
	dtypeMatch := regexp.MustCompile(`'descr':\s*'([^']*)'`).FindStringSubmatch(header)
	if len(dtypeMatch) < 2 {
		return nil, "", nil, errors.New("failed to find descr in NPY header")
	}
	dtype = dtypeMatch[1]

	// Parse shape
	shapeMatch := regexp.MustCompile(`'shape':\s*\(([^)]*)\)`).FindStringSubmatch(header)
	if len(shapeMatch) < 2 {
		return nil, "", nil, errors.New("failed to find shape in NPY header")
	}
	shapeStr := shapeMatch[1]
	shapeParts := strings.Split(shapeStr, ",")
	for _, part := range shapeParts {
		part = strings.TrimSpace(part)
		if part == "" {
			continue
		}
		val, err := strconv.Atoi(part)
		if err != nil {
			return nil, "", nil, fmt.Errorf("failed to parse shape dimension %q: %w", part, err)
		}
		shape = append(shape, val)
	}

	// Rest is data
	data, err = io.ReadAll(r)
	if err != nil {
		return nil, "", nil, err
	}

	return shape, dtype, data, nil
}

// LoadMNISTNPZ loads all MNIST test samples from the given .npz file.
// It normalizes the images using (val/255.0 - 0.1307)/0.3081.
func LoadMNISTNPZ(npzPath string) (images [][]float32, labels []int, err error) {
	r, err := zip.OpenReader(npzPath)
	if err != nil {
		return nil, nil, err
	}
	defer r.Close()

	var xFile, yFile *zip.File
	for _, f := range r.File {
		if f.Name == "x_test.npy" {
			xFile = f
		} else if f.Name == "y_test.npy" {
			yFile = f
		}
	}

	if xFile == nil || yFile == nil {
		return nil, nil, errors.New("x_test.npy or y_test.npy not found in npz")
	}

	// Load images
	xReader, err := xFile.Open()
	if err != nil {
		return nil, nil, err
	}
	defer xReader.Close()
	xShape, xDtype, xData, err := parseNPY(xReader)
	if err != nil {
		return nil, nil, fmt.Errorf("failed to parse x_test.npy: %w", err)
	}

	// Load labels
	yReader, err := yFile.Open()
	if err != nil {
		return nil, nil, err
	}
	defer yReader.Close()
	yShape, yDtype, yData, err := parseNPY(yReader)
	if err != nil {
		return nil, nil, fmt.Errorf("failed to parse y_test.npy: %w", err)
	}

	// Verify shapes
	if len(xShape) < 2 {
		return nil, nil, fmt.Errorf("invalid x_test shape: %v", xShape)
	}
	numSamples := xShape[0]
	imageSize := 1
	for _, dim := range xShape[1:] {
		imageSize *= dim
	}
	if imageSize != 784 {
		return nil, nil, fmt.Errorf("expected image size 784, got %d", imageSize)
	}

	if len(yShape) != 1 || yShape[0] != numSamples {
		return nil, nil, fmt.Errorf("invalid y_test shape %v for %d samples", yShape, numSamples)
	}

	// Parse labels
	labels = make([]int, numSamples)
	if yDtype == "|u1" || yDtype == "|i1" {
		for i := 0; i < numSamples; i++ {
			labels[i] = int(yData[i])
		}
	} else if yDtype == "<i8" {
		if len(yData) < numSamples*8 {
			return nil, nil, fmt.Errorf("insufficient data for int64 labels: got %d bytes, want %d", len(yData), numSamples*8)
		}
		for i := 0; i < numSamples; i++ {
			val := binary.LittleEndian.Uint64(yData[i*8 : (i+1)*8])
			labels[i] = int(val)
		}
	} else {
		return nil, nil, fmt.Errorf("unsupported y_test dtype: %s", yDtype)
	}

	// Parse and normalize images
	images = make([][]float32, numSamples)
	if xDtype == "|u1" {
		if len(xData) < numSamples*imageSize {
			return nil, nil, fmt.Errorf("insufficient data for images: got %d bytes, want %d", len(xData), numSamples*imageSize)
		}
		for i := 0; i < numSamples; i++ {
			img := make([]float32, imageSize)
			offset := i * imageSize
			for j := 0; j < imageSize; j++ {
				val := float64(xData[offset+j]) / 255.0
				img[j] = float32((val - 0.1307) / 0.3081)
			}
			images[i] = img
		}
	} else {
		return nil, nil, fmt.Errorf("unsupported x_test dtype: %s", xDtype)
	}

	return images, labels, nil
}

// LoadMNISTSampleNPZ loads a single MNIST test sample from the given .npz file.
func LoadMNISTSampleNPZ(npzPath string, idx int) (image []float32, label int, err error) {
	images, labels, err := LoadMNISTNPZ(npzPath)
	if err != nil {
		return nil, 0, err
	}
	if idx < 0 || idx >= len(images) {
		return nil, 0, fmt.Errorf("sample index %d out of bounds (total %d)", idx, len(images))
	}
	return images[idx], labels[idx], nil
}
