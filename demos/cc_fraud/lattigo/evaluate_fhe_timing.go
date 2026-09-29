package main

import (
	"flag"
	"fmt"
	"os"
	"time"

	"fully_homomorphic_encryption/demos/cc_fraud/lattigo/fraud_model_lattigo_timing"
	"fully_homomorphic_encryption/demos/cc_fraud/lattigo/fraud_model_lattigo_timing_utils"
	"fully_homomorphic_encryption/demos/common/go/pathutils"
)

func main() {
	rowIdxFlag := flag.Int("row_idx", 0, "Row index in the CSV to test")
	csvPathFlag := flag.String("csv_path", "test_rows.csv", "Path to the test CSV file")
	flag.Parse()

	csvPath := *csvPathFlag
	if csvPath == "test_rows.csv" {
		csvPath = pathutils.ResolvePath("fully_homomorphic_encryption/demos/cc_fraud/data/test_rows.csv")
	}
	rowIdx := *rowIdxFlag

	fmt.Printf("Loading test row %d from %s...\n", rowIdx, csvPath)
	t0 := time.Now()
	features, expectedLabel, err := loadTestRow(csvPath, rowIdx)
	if err != nil {
		fmt.Printf("Error loading test row: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("  Took %v\n", time.Since(t0))
	fmt.Printf("  Expected label (is_fraud): %d\n", expectedLabel)
	fmt.Printf("  Feature vector size: %d\n", len(features))
	fmt.Printf("  First 5 features: %v\n", features[:5])

	// Configure context
	fmt.Println("Configuring Lattigo context...")
	t0 = time.Now()
	evaluator, params, ecd, encryptor, decryptor := fraud_model_lattigo_timing.Cc_fraud__configure()
	fmt.Printf("  Took %v\n", time.Since(t0))

	// Encrypt input
	fmt.Println("Encrypting input features...")
	t0 = time.Now()
	encryptedFeatures := fraud_model_lattigo_timing.Cc_fraud__encrypt__arg0(evaluator, params, ecd, encryptor, features)
	fmt.Printf("  Took %v\n", time.Since(t0))

	// Preprocessing
	fmt.Println("Running preprocessing...")
	t0 = time.Now()
	preprocessedWeights := fraud_model_lattigo_timing_utils.Cc_fraud__preprocessing(params, ecd)
	fmt.Printf("  Took %v\n", time.Since(t0))

	// FHE evaluation (preprocessed with timing callbacks)
	fmt.Println("Running FHE evaluation (preprocessed with timing callbacks)...")
	t0 = time.Now()
	ctZeros := fraud_model_lattigo_timing.Cc_fraud__encrypt__zeros(evaluator, params, ecd, encryptor)
	encryptedOutput := fraud_model_lattigo_timing.Cc_fraud__preprocessed(
		evaluator, params, ecd, decryptor, encryptedFeatures,
		ctZeros,
		preprocessedWeights,
	)
	fmt.Printf("  Took %v\n", time.Since(t0))
	fmt.Printf("  MaxLevel: %d, Output Level: %d\n", params.MaxLevel(), encryptedOutput[0].Level())

	// Decrypt
	fmt.Println("Decrypting output...")
	t0 = time.Now()
	decryptedLogits := fraud_model_lattigo_timing.Cc_fraud__decrypt__result0(evaluator, params, ecd, decryptor, encryptedOutput)
	fmt.Printf("  Took %v\n", time.Since(t0))

	fmt.Printf("Decrypted logits: %v\n", decryptedLogits)

	// Argmax
	predictedClass := 0
	if decryptedLogits[1] > decryptedLogits[0] {
		predictedClass = 1
	}
	fmt.Printf("Predicted class: %d\n", predictedClass)

	if predictedClass == expectedLabel {
		fmt.Println("SUCCESS: Predicted class matches expected label!")
	} else {
		fmt.Println("FAILURE: Predicted class does NOT match expected label!")
		os.Exit(1)
	}
}
