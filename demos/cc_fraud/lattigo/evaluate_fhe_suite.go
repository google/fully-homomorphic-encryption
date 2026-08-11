package main

import (
	"fmt"
	"os"
	"sync"
	"time"

	"fully_homomorphic_encryption/demos/cc_fraud/lattigo/fraud_model_lattigo"
	"fully_homomorphic_encryption/demos/cc_fraud/lattigo/fraud_model_lattigo_utils"
	"fully_homomorphic_encryption/demos/common/go/pathutils"
	"github.com/tuneinsight/lattigo/v6/core/rlwe"
)

func main() {
	csvPath := "test_rows.csv"
	if csvPath == "test_rows.csv" {
		csvPath = pathutils.ResolvePath("fully_homomorphic_encryption/demos/cc_fraud/data/test_rows.csv")
	}

	fmt.Printf("Loading all test rows from %s...\n", csvPath)
	t0 := time.Now()
	allFeatures, expectedLabels, err := loadAllTestRows(csvPath)
	if err != nil {
		fmt.Printf("Error loading test rows: %v\n", err)
		os.Exit(1)
	}
	numRows := len(allFeatures)
	fmt.Printf("  Loaded %d rows in %v\n", numRows, time.Since(t0))

	// Configure context (ONCE)
	fmt.Println("Configuring Lattigo context...")
	t0 = time.Now()
	evaluator, params, ecd, encryptor, decryptor := fraud_model_lattigo.Cc_fraud__configure()
	fmt.Printf("  Took %v\n", time.Since(t0))

	// Preprocessing (ONCE)
	fmt.Println("Running preprocessing for model weights...")
	t0 = time.Now()
	preprocessedWeights := fraud_model_lattigo_utils.Cc_fraud__preprocessing(params, ecd)
	fmt.Printf("  Took %v\n", time.Since(t0))

	// 1. Sequential Encryption (Encryptor is typically not thread-safe)
	fmt.Println("Encrypting all input features and zero accumulators sequentially...")
	t0 = time.Now()
	encryptedInputs := make([][]*rlwe.Ciphertext, numRows)
	ctZeros1 := make([]*rlwe.Ciphertext, numRows)
	ctZeros2 := make([]*rlwe.Ciphertext, numRows)
	for i := 0; i < numRows; i++ {
		encryptedInputs[i] = fraud_model_lattigo.Cc_fraud__encrypt__arg0(evaluator, params, ecd, encryptor, allFeatures[i])
		ctZeros1[i] = fraud_model_lattigo.Cc_fraud__encrypt__zero__0(evaluator, params, ecd, encryptor)
		ctZeros2[i] = fraud_model_lattigo.Cc_fraud__encrypt__zero__1(evaluator, params, ecd, encryptor)
	}
	fmt.Printf("  Took %v\n", time.Since(t0))

	// 2. Parallel FHE Evaluation (Using ShallowCopy for thread safety)
	fmt.Println("\nStarting parallel FHE evaluation suite...")
	encryptedOutputs := make([][]*rlwe.Ciphertext, numRows)

	var wg sync.WaitGroup
	wg.Add(numRows)

	suiteStartTime := time.Now()
	for i := 0; i < numRows; i++ {
		go func(idx int) {
			defer wg.Done()
			localEvaluator := evaluator.ShallowCopy()
			encryptedOutputs[idx] = fraud_model_lattigo.Cc_fraud__preprocessed(
				localEvaluator, params, ecd, encryptedInputs[idx],
				ctZeros1[idx], ctZeros2[idx],
				preprocessedWeights,
			)
		}(i)
	}

	wg.Wait()
	totalEvalTime := time.Since(suiteStartTime)
	fmt.Printf("  Parallel evaluation completed in %v (average %v per row, wall time)\n", totalEvalTime, totalEvalTime/time.Duration(numRows))

	// 3. Sequential Decryption & Verification (Decryptor is typically not thread-safe)
	fmt.Println("\nDecrypting and verifying results sequentially...")
	correctCount := 0
	var misclassifications []struct {
		idx  int
		exp  int
		pred int
	}

	for idx := 0; idx < numRows; idx++ {
		decryptedLogits := fraud_model_lattigo.Cc_fraud__decrypt__result0(evaluator, params, ecd, decryptor, encryptedOutputs[idx])

		// Argmax
		predictedClass := 0
		if decryptedLogits[1] > decryptedLogits[0] {
			predictedClass = 1
		}

		expectedLabel := expectedLabels[idx]
		isCorrect := (predictedClass == expectedLabel)
		status := "MISCLASSIFIED"
		if isCorrect {
			status = "SUCCESS"
			correctCount++
		} else {
			misclassifications = append(misclassifications, struct {
				idx  int
				exp  int
				pred int
			}{idx, expectedLabel, predictedClass})
		}

		fmt.Printf("Row %3d: expected %d, got %d (%s)\n", idx, expectedLabel, predictedClass, status)
	}

	accuracy := float64(correctCount) / float64(numRows)
	fmt.Printf("\nAccuracy: %d/%d (%.2f%%)\n", correctCount, numRows, accuracy*100)

	if len(misclassifications) > 0 {
		fmt.Println("\nSummary of Misclassifications:")
		for _, m := range misclassifications {
			fmt.Printf("  Row %3d: expected %d, got %d\n", m.idx, m.exp, m.pred)
		}
	} else {
		fmt.Println("\nNO MISCLASSIFICATIONS!")
	}
}
