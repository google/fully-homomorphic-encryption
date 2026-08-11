// Package utils provides utilities for loading data used to evaluate models.
package utils

import (
	"encoding/binary"
	"encoding/csv"
	"fmt"
	"io"
	"math"
	"os"
	"strconv"

	"fully_homomorphic_encryption/demos/common/go/pathutils"
)

// LoadPacketSample reads a single packet sample from a binary double (float64) dataset file.
func LoadPacketSample(dataPath string, sampleIdx int, numFeatures int) ([]float32, error) {
	resolvedPath := pathutils.ResolvePath(dataPath)
	file, err := os.Open(resolvedPath)
	if err != nil {
		return nil, fmt.Errorf("failed to open dataset file: %w", err)
	}
	defer file.Close()

	bytesPerSample := int64(numFeatures * 8)
	offset := int64(sampleIdx) * bytesPerSample

	_, err = file.Seek(offset, io.SeekStart)
	if err != nil {
		return nil, fmt.Errorf("failed to seek to sample %d: %w", sampleIdx, err)
	}

	buf := make([]byte, bytesPerSample)
	n, err := io.ReadFull(file, buf)
	if err != nil {
		return nil, fmt.Errorf("failed to read sample %d (read %d bytes): %w", sampleIdx, n, err)
	}

	features := make([]float32, numFeatures)
	for i := 0; i < numFeatures; i++ {
		bits := binary.LittleEndian.Uint64(buf[i*8 : (i+1)*8])
		val := math.Float64frombits(bits)
		features[i] = float32(val)
	}

	return features, nil
}

// LoadAllPacketSamples reads up to maxSamples from a binary double (float64) dataset file.
func LoadAllPacketSamples(dataPath string, maxSamples int, numFeatures int) ([][]float32, error) {
	resolvedPath := pathutils.ResolvePath(dataPath)
	file, err := os.Open(resolvedPath)
	if err != nil {
		return nil, fmt.Errorf("failed to open dataset file: %w", err)
	}
	defer file.Close()

	bytesPerSample := numFeatures * 8
	buf := make([]byte, bytesPerSample)
	var samples [][]float32

	for i := 0; i < maxSamples; i++ {
		_, err := io.ReadFull(file, buf)
		if err == io.EOF || err == io.ErrUnexpectedEOF {
			break
		}
		if err != nil {
			return nil, fmt.Errorf("failed reading sample %d: %w", i, err)
		}

		features := make([]float32, numFeatures)
		for j := 0; j < numFeatures; j++ {
			bits := binary.LittleEndian.Uint64(buf[j*8 : (j+1)*8])
			val := math.Float64frombits(bits)
			features[j] = float32(val)
		}
		samples = append(samples, features)
	}

	return samples, nil
}

// LoadLabels reads binary anomaly labels (0=benign, 1=anomaly) from a CSV file.
func LoadLabels(labelsPath string, maxSamples int) ([]int, error) {
	resolvedPath := pathutils.ResolvePath(labelsPath)
	file, err := os.Open(resolvedPath)
	if err != nil {
		return nil, fmt.Errorf("failed to open labels file: %w", err)
	}
	defer file.Close()

	reader := csv.NewReader(file)
	// Read header
	_, err = reader.Read()
	if err != nil {
		return nil, fmt.Errorf("failed to read CSV header: %w", err)
	}

	var labels []int
	for len(labels) < maxSamples {
		record, err := reader.Read()
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, fmt.Errorf("failed reading label record: %w", err)
		}

		var labelStr string
		if len(record) >= 2 {
			labelStr = record[1]
		} else if len(record) == 1 {
			labelStr = record[0]
		} else {
			continue
		}

		val, err := strconv.ParseFloat(labelStr, 64)
		if err != nil {
			continue
		}
		labels = append(labels, int(val))
	}

	return labels, nil
}

// ConfusionMatrix represents standard binary classification metrics.
type ConfusionMatrix struct {
	TP          int
	TN          int
	FP          int
	FN          int
	Total       int
	Accuracy    float64
	Specificity float64
	FPR         float64
}

// CalculateConfusionMatrix computes classification performance against ground truth labels.
func CalculateConfusionMatrix(labels []int, isAnomaly []bool) ConfusionMatrix {
	var cm ConfusionMatrix
	cm.Total = len(labels)
	for i := 0; i < len(labels); i++ {
		actual := labels[i]
		pred := isAnomaly[i]

		if actual == 1 && pred {
			cm.TP++
		} else if actual == 0 && !pred {
			cm.TN++
		} else if actual == 0 && pred {
			cm.FP++
		} else if actual == 1 && !pred {
			cm.FN++
		}
	}

	if cm.Total > 0 {
		cm.Accuracy = float64(cm.TP+cm.TN) / float64(cm.Total) * 100.0
	}
	if cm.TN+cm.FP > 0 {
		cm.Specificity = float64(cm.TN) / float64(cm.TN+cm.FP) * 100.0
		cm.FPR = float64(cm.FP) / float64(cm.TN+cm.FP) * 100.0
	}
	return cm
}
