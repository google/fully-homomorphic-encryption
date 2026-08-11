package main

import (
	"encoding/csv"
	"fmt"
	"io"
	"os"
	"strconv"
)

func parseRecord(record []string) ([]float32, int, error) {
	label, err := strconv.Atoi(record[0])
	if err != nil {
		return nil, 0, fmt.Errorf("invalid label: %v", err)
	}

	features := make([]float32, len(record)-1)
	for i := 1; i < len(record); i++ {
		val, err := strconv.ParseFloat(record[i], 32)
		if err != nil {
			return nil, 0, fmt.Errorf("invalid feature at col %d: %v", i, err)
		}
		features[i-1] = float32(val)
	}
	return features, label, nil
}

func loadTestRow(csvPath string, targetRowIdx int) ([]float32, int, error) {
	file, err := os.Open(csvPath)
	if err != nil {
		return nil, 0, err
	}
	defer file.Close()

	reader := csv.NewReader(file)
	// Read header
	_, err = reader.Read()
	if err != nil {
		return nil, 0, err
	}

	currentRowIdx := 0
	for {
		record, err := reader.Read()
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, 0, err
		}

		if currentRowIdx == targetRowIdx {
			return parseRecord(record)
		}
		currentRowIdx++
	}

	return nil, 0, fmt.Errorf("row index %d out of bounds", targetRowIdx)
}

func loadAllTestRows(csvPath string) ([][]float32, []int, error) {
	file, err := os.Open(csvPath)
	if err != nil {
		return nil, nil, err
	}
	defer file.Close()

	reader := csv.NewReader(file)
	// Read header
	_, err = reader.Read()
	if err != nil {
		return nil, nil, err
	}

	var allFeatures [][]float32
	var labels []int

	for {
		record, err := reader.Read()
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, nil, err
		}

		features, label, err := parseRecord(record)
		if err != nil {
			return nil, nil, err
		}

		allFeatures = append(allFeatures, features)
		labels = append(labels, label)
	}

	return allFeatures, labels, nil
}
