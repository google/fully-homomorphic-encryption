package main

import (
	"flag"
	"fmt"
	"os"
	"time"

	"fully_homomorphic_encryption/demos/network_anomaly/lattigo/anomaly_model_lattigo_timing"
	"fully_homomorphic_encryption/demos/network_anomaly/lattigo/anomaly_model_lattigo_timing_utils"
	"fully_homomorphic_encryption/demos/network_anomaly/lattigo/utils"
)

func main() {
	sampleIdxFlag := flag.Int("sample_idx", 0, "Packet sample index to benchmark")
	dataPathFlag := flag.String(
		"data_path",
		"fully_homomorphic_encryption/demos/network_anomaly/data/Mirai_first_batch_32K.bin",
		"Path to binary double (float64) dataset file",
	)
	runsFlag := flag.Int("runs", 3, "Number of repeated timing iterations")
	flag.Parse()

	sampleIdx := *sampleIdxFlag
	dataPath := *dataPathFlag
	runs := *runsFlag
	numFeatures := 5

	fmt.Println("================================================================================")
	fmt.Println("  PyTorch KitNET Lattigo FHE Phase Timing & Benchmark")
	fmt.Println("================================================================================")
	fmt.Printf("Dataset File:       %s\n", dataPath)
	fmt.Printf("Sample Index:       %d\n", sampleIdx)
	fmt.Printf("Number of Runs:     %d\n\n", runs)

	// 1. Data Loading
	t0 := time.Now()
	features, err := utils.LoadPacketSample(dataPath, sampleIdx, numFeatures)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error loading sample: %v\n", err)
		os.Exit(1)
	}
	loadDur := time.Since(t0)
	fmt.Printf("[Phase 1] Data Loading:           %10v\n", loadDur)

	// 2. Context & Keys Configuration
	t0 = time.Now()
	evaluator, params, encoder, encryptor, decryptor := anomaly_model_lattigo_timing.Main__configure()
	configDur := time.Since(t0)
	fmt.Printf("[Phase 2] Lattigo Context Setup:  %10v (N: %d, MaxLevel: %d)\n",
		configDur, params.N(), params.MaxLevel())

	// 3. Weight Preprocessing
	t0 = time.Now()
	preprocessedPlaintexts := anomaly_model_lattigo_timing_utils.Main__preprocessing(params, encoder)
	prepDur := time.Since(t0)
	fmt.Printf("[Phase 3] Weight Preprocessing:   %10v (%d plaintexts)\n",
		prepDur, len(preprocessedPlaintexts))

	// 4. Repeated Evaluation Runs
	fmt.Printf("\n--- Running %d FHE Evaluation Iterations ---\n", runs)
	var totalEnc, totalFhe, totalDec time.Duration
	var lastRawSSE float64

	for r := 1; r <= runs; r++ {
		t0 = time.Now()
		encryptedInput := anomaly_model_lattigo_timing.Main__encrypt__arg0(evaluator, params, encoder, encryptor, features)
		encDur := time.Since(t0)
		totalEnc += encDur

		t0 = time.Now()
		res0, _ := anomaly_model_lattigo_timing.Main__preprocessed(
			evaluator, params, encoder, decryptor, encryptedInput, preprocessedPlaintexts,
		)
		fheDur := time.Since(t0)
		totalFhe += fheDur

		t0 = time.Now()
		decryptedSSE := anomaly_model_lattigo_timing.Main__decrypt__result0(evaluator, params, encoder, decryptor, res0)
		decDur := time.Since(t0)
		totalDec += decDur
		lastRawSSE = float64(decryptedSSE[0])

		fmt.Printf("  Run %d/%d -> Encrypt: %8v | FHE Eval: %8v | Decrypt: %8v | Total: %8v\n",
			r, runs, encDur, fheDur, decDur, encDur+fheDur+decDur)
	}

	avgEnc := totalEnc / time.Duration(runs)
	avgFhe := totalFhe / time.Duration(runs)
	avgDec := totalDec / time.Duration(runs)
	avgTotal := avgEnc + avgFhe + avgDec

	fmt.Println("\n================================================================================")
	fmt.Println("  Timing Benchmark Summary (Averages over runs)")
	fmt.Println("================================================================================")
	fmt.Printf("  • Encryption Latency:         %10v\n", avgEnc)
	fmt.Printf("  • FHE Evaluation Latency:     %10v\n", avgFhe)
	fmt.Printf("  • Decryption Latency:         %10v\n", avgDec)
	fmt.Printf("  --------------------------------------------------\n")
	fmt.Printf("  • Total End-to-End Latency:   %10v\n", avgTotal)
	fmt.Printf("  • Decrypted SSE Score:        %e (MSE: %e)\n", lastRawSSE, lastRawSSE/float64(numFeatures))
	fmt.Println("================================================================================")
}
