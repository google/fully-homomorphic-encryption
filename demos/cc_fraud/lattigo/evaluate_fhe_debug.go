package main

import (
	"flag"
	"fmt"
	"os"
	"strconv"
	"time"

	"fully_homomorphic_encryption/demos/cc_fraud/lattigo/fraud_model_lattigo_debug"
	"fully_homomorphic_encryption/demos/cc_fraud/lattigo/fraud_model_lattigo_debug_utils"
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

	// Set environment variable for the debug helper
	os.Setenv("HEIR_DEBUG_ROW_IDX", strconv.Itoa(rowIdx))

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
	evaluator, params, ecd, encryptor, decryptor := fraud_model_lattigo_debug.Cc_fraud__configure()
	fmt.Printf("  Took %v\n", time.Since(t0))

	// Encrypt input
	fmt.Println("Encrypting input features...")
	t0 = time.Now()
	encryptedFeatures := fraud_model_lattigo_debug.Cc_fraud__encrypt__arg0(evaluator, params, ecd, encryptor, features)
	fmt.Printf("  Took %v\n", time.Since(t0))

	// Preprocessing
	fmt.Println("Running preprocessing...")
	t0 = time.Now()
	preprocessedWeights := fraud_model_lattigo_debug_utils.Cc_fraud__preprocessing(params, ecd)
	fmt.Printf("  Took %v\n", time.Since(t0))

	// FHE evaluation (with debug callbacks)
	fmt.Println("\n--- Starting FHE Evaluation (with Debug Callbacks) ---")
	t0 = time.Now()
	ctZero1 := fraud_model_lattigo_debug.Cc_fraud__encrypt__zero__0(evaluator, params, ecd, encryptor)
	ctZero2 := fraud_model_lattigo_debug.Cc_fraud__encrypt__zero__1(evaluator, params, ecd, encryptor)
	encryptedOutput := fraud_model_lattigo_debug.Cc_fraud__preprocessed(
		evaluator, params, ecd, decryptor, encryptedFeatures,
		ctZero1, ctZero2,
		preprocessedWeights,
	)
	fmt.Printf("--- FHE Evaluation Completed in %v ---\n\n", time.Since(t0))

	// Decrypt
	fmt.Println("Decrypting final output...")
	t0 = time.Now()
	decryptedLogits := fraud_model_lattigo_debug.Cc_fraud__decrypt__result0(evaluator, params, ecd, decryptor, encryptedOutput)
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
