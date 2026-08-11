package main

import (
	"flag"
	"fmt"
	"os"
	"time"

	"fully_homomorphic_encryption/demos/network_anomaly/lattigo/anomaly_model_lattigo"
	"fully_homomorphic_encryption/demos/network_anomaly/lattigo/anomaly_model_lattigo_utils"
	"fully_homomorphic_encryption/demos/network_anomaly/lattigo/utils"
)

func main() {
	sampleIdxFlag := flag.Int("sample_idx", 0, "Zero-based packet sample index to evaluate")
	dataPathFlag := flag.String(
		"data_path",
		"fully_homomorphic_encryption/demos/network_anomaly/data/Mirai_first_batch_32K.bin",
		"Path to binary double (float64) dataset file",
	)
	verboseFlag := flag.Bool("verbose", true, "Print detailed vectors")
	flag.Parse()

	sampleIdx := *sampleIdxFlag
	dataPath := *dataPathFlag
	numFeatures := 5

	fmt.Println("================================================================================")
	fmt.Println("  PyTorch KitNET Lattigo FHE Single Sample Evaluation")
	fmt.Println("================================================================================")
	fmt.Printf("Dataset File:       %s\n", dataPath)
	fmt.Printf("Sample Index:       %d\n", sampleIdx)
	fmt.Printf("Number of Features: %d\n\n", numFeatures)

	// 1. Load Packet Sample
	fmt.Println("[1/5] Loading sample from binary dataset...")
	t0 := time.Now()
	features, err := utils.LoadPacketSample(dataPath, sampleIdx, numFeatures)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error loading packet sample: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("  Loaded %d features in %v\n", len(features), time.Since(t0))
	if *verboseFlag {
		fmt.Printf("  Input Features: %v\n", features)
	}

	// 2. Configure Lattigo CKKS Context
	fmt.Println("\n[2/5] Configuring Lattigo CKKS cryptocontext & keys...")
	t0 = time.Now()
	evaluator, params, encoder, encryptor, decryptor := anomaly_model_lattigo.Main__configure()
	fmt.Printf("  Lattigo context ready in %v (Ring Degree N: %d, Max Slots: %d)\n",
		time.Since(t0), params.N(), params.MaxSlots())

	// 3. Preprocess Weights
	fmt.Println("\n[3/5] Preprocessing model weights into plaintexts...")
	t0 = time.Now()
	preprocessedPlaintexts := anomaly_model_lattigo_utils.Main__preprocessing(params, encoder)
	fmt.Printf("  Preprocessed %d weight plaintexts in %v\n", len(preprocessedPlaintexts), time.Since(t0))

	// 4. Encrypt Input Features
	fmt.Println("\n[4/5] Encrypting input features into CKKS ciphertext...")
	t0 = time.Now()
	encryptedInput := anomaly_model_lattigo.Main__encrypt__arg0(evaluator, params, encoder, encryptor, features)
	fmt.Printf("  Encrypted %d ciphertext(s) in %v\n", len(encryptedInput), time.Since(t0))

	// 5. Evaluate FHE Circuit
	fmt.Println("\n[5/5] Executing FHE evaluation (Ensemble + Anomaly Detector AutoEncoders)...")
	t0 = time.Now()
	encryptedRes0, encryptedRes1 := anomaly_model_lattigo.Main__preprocessed(
		evaluator, params, encoder, encryptedInput, preprocessedPlaintexts,
	)
	fheDuration := time.Since(t0)
	fmt.Printf("  FHE Evaluation completed in %v\n", fheDuration)

	// 6. Decrypt Results
	fmt.Println("\n--- Decryption & Anomaly Scoring ---")
	t0 = time.Now()
	decryptedSSE := anomaly_model_lattigo.Main__decrypt__result0(evaluator, params, encoder, decryptor, encryptedRes0)
	decryptedResiduals := anomaly_model_lattigo.Main__decrypt__result1(evaluator, params, encoder, decryptor, encryptedRes1)
	decryptDuration := time.Since(t0)
	fmt.Printf("  Decryption took %v\n", decryptDuration)

	rawSSE := float64(decryptedSSE[0])
	anomalyMSE := rawSSE / float64(numFeatures)

	fmt.Println("\n================================================================================")
	fmt.Println("  Evaluation Results")
	fmt.Println("================================================================================")
	fmt.Printf("Sample Index:            %d\n", sampleIdx)
	fmt.Printf("Decrypted Raw SSE Score: %e\n", rawSSE)
	fmt.Printf("Anomaly MSE Score:       %e  (SSE / %d features)\n", anomalyMSE, numFeatures)
	fmt.Printf("FHE Inference Latency:   %v\n", fheDuration)
	if *verboseFlag {
		fmt.Printf("Output Residual Vector:  %v\n", decryptedResiduals[:numFeatures])
	}
	fmt.Println("================================================================================")
}
