// Package main provides a batched evaluation suite runner for the Lattigo KitNET FHE network anomaly detection model.
package main

import (
	"flag"
	"fmt"
	"math"
	"os"
	"time"

	"fully_homomorphic_encryption/demos/network_anomaly/lattigo/anomaly_model_lattigo"
	"fully_homomorphic_encryption/demos/network_anomaly/lattigo/anomaly_model_lattigo_utils"
	"fully_homomorphic_encryption/demos/network_anomaly/lattigo/utils"
	"github.com/tuneinsight/lattigo/v6/core/rlwe"
)

func main() {
	numSamplesFlag := flag.Int("num_samples", 10, "Number of packet samples to evaluate in suite")
	dataPathFlag := flag.String(
		"data_path",
		"fully_homomorphic_encryption/demos/network_anomaly/data/Mirai_first_batch_32K.bin",
		"Path to binary double (float64) dataset file",
	)
	labelsPathFlag := flag.String(
		"labels_path",
		"fully_homomorphic_encryption/demos/network_anomaly/data/Mirai_labels.csv",
		"Path to ground truth labels CSV file",
	)
	thresholdFlag := flag.Float64("threshold", 0.005, "Anomaly detection MSE threshold")
	flag.Parse()

	numSamples := *numSamplesFlag
	dataPath := *dataPathFlag
	labelsPath := *labelsPathFlag
	threshold := *thresholdFlag
	numFeatures := 5

	fmt.Println("================================================================================")
	fmt.Println("  PyTorch KitNET Lattigo FHE Multi-Sample Suite Evaluation")
	fmt.Println("================================================================================")
	fmt.Printf("Dataset File:       %s\n", dataPath)
	fmt.Printf("Labels File:        %s\n", labelsPath)
	fmt.Printf("Target Samples:     %d\n", numSamples)
	fmt.Printf("Number of Features: %d\n", numFeatures)
	fmt.Printf("Anomaly Threshold:  %e\n\n", threshold)

	// 1. Load Packet Samples
	fmt.Println("[1/4] Loading packet samples...")
	t0 := time.Now()
	allSamples, err := utils.LoadAllPacketSamples(dataPath, numSamples, numFeatures)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error loading packet samples: %v\n", err)
		os.Exit(1)
	}
	actualSamples := len(allSamples)
	fmt.Printf("  Loaded %d samples in %v\n", actualSamples, time.Since(t0))

	// 2. Load Ground Truth Labels
	labels, err := utils.LoadLabels(labelsPath, actualSamples)
	if err != nil {
		fmt.Printf("  Notice: Could not load labels: %v\n", err)
	} else {
		fmt.Printf("  Loaded %d ground truth labels\n", len(labels))
	}

	// 3. Configure Lattigo CKKS Context
	fmt.Println("\n[2/4] Initializing Lattigo CKKS cryptocontext & keys...")
	t0 = time.Now()
	evaluator, params, encoder, encryptor, decryptor := anomaly_model_lattigo.Main__configure()
	fmt.Printf("  Context ready in %v\n", time.Since(t0))

	// 4. Preprocess Weights
	fmt.Println("\n[3/4] Preprocessing weights into plaintexts...")
	t0 = time.Now()
	preprocessedPlaintexts := anomaly_model_lattigo_utils.Main__preprocessing(params, encoder)
	fmt.Printf("  Preprocessed %d weight plaintexts in %v\n", len(preprocessedPlaintexts), time.Since(t0))

	// 5. Evaluate FHE Loop
	fmt.Println("\n[4/4] Evaluating encrypted samples...")
	fheScores := make([]float64, actualSamples)
	isAnomaly := make([]bool, actualSamples)

	suiteStart := time.Now()
	for i := 0; i < actualSamples; i++ {
		sampleStart := time.Now()
		encryptedInput := anomaly_model_lattigo.Main__encrypt__arg0(evaluator, params, encoder, encryptor, allSamples[i])
		res0, _ := anomaly_model_lattigo.Main__preprocessed(
			evaluator, params, encoder, encryptedInput, preprocessedPlaintexts,
		)
		decryptedSSE := anomaly_model_lattigo.Main__decrypt__result0(evaluator, params, encoder, decryptor, res0)
		rawSSE := float64(decryptedSSE[0])
		anomalyMSE := rawSSE / float64(numFeatures)
		fheScores[i] = anomalyMSE
		isAnomaly[i] = anomalyMSE >= threshold

		sampleDur := time.Since(sampleStart)
		flagStr := "BENIGN"
		if isAnomaly[i] {
			flagStr = "ANOMALY"
		}
		fmt.Printf("  Sample [%2d/%2d] -> FHE MSE: %11.6e | Result: %-7s | Latency: %v\n",
			i+1, actualSamples, anomalyMSE, flagStr, sampleDur)
	}
	totalFheDuration := time.Since(suiteStart)

	// Summary Statistics
	var sumScore, minScore, maxScore float64
	minScore = math.MaxFloat64
	anomCount := 0
	for _, s := range fheScores {
		sumScore += s
		if s < minScore {
			minScore = s
		}
		if s > maxScore {
			maxScore = s
		}
		if s >= threshold {
			anomCount++
		}
	}
	avgScore := sumScore / float64(actualSamples)

	fmt.Println("\n================================================================================")
	fmt.Println("  Suite FHE Evaluation Summary")
	fmt.Println("================================================================================")
	fmt.Printf("Total Samples Evaluated:   %d\n", actualSamples)
	fmt.Printf("Average Anomaly MSE Score: %e\n", avgScore)
	fmt.Printf("Min Anomaly MSE Score:     %e\n", minScore)
	fmt.Printf("Max Anomaly MSE Score:     %e\n", maxScore)
	fmt.Printf("Packets Flagged Anomaly:   %d / %d (%.2f%%)\n",
		anomCount, actualSamples, float64(anomCount)/float64(actualSamples)*100.0)
	fmt.Printf("Total Evaluation Time:     %v\n", totalFheDuration)
	fmt.Printf("Average FHE Latency:       %v / sample\n", totalFheDuration/time.Duration(actualSamples))

	if labels != nil && len(labels) == actualSamples {
		cm := utils.CalculateConfusionMatrix(labels, isAnomaly)
		fmt.Println("\n--- Ground Truth Validation & Confusion Matrix ---")
		fmt.Printf("  • True Positives  (TP): %d\n", cm.TP)
		fmt.Printf("  • True Negatives  (TN): %d\n", cm.TN)
		fmt.Printf("  • False Positives (FP): %d\n", cm.FP)
		fmt.Printf("  • False Negatives (FN): %d\n", cm.FN)
		fmt.Printf("  • Accuracy:             %.2f%%\n", cm.Accuracy)
		fmt.Printf("  • Specificity:          %.2f%%\n", cm.Specificity)
	}
	fmt.Println("================================================================================")

	// Avoid compiler unused warning
	_ = rlwe.Ciphertext{}
}
