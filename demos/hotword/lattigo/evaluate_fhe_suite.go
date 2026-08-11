// Evaluate inference on a dataset
package main

import (
	"flag"
	"fmt"
	"os"
	"sync"
	"time"

	"fully_homomorphic_encryption/demos/common/go/pathutils"
	"fully_homomorphic_encryption/demos/hotword/lattigo/hotword_lattigo"
	"fully_homomorphic_encryption/demos/hotword/lattigo/hotword_lattigo_utils"
	"github.com/tuneinsight/lattigo/v6/core/rlwe"
)

var labels = [12]string{
	"_silence_",
	"_unknown_",
	"yes",
	"no",
	"up",
	"down",
	"left",
	"right",
	"on",
	"off",
	"stop",
	"go",
}

func main() {
	npzPathFlag := flag.String("npz_path", "test_data.npz", "Path to the test NPZ file")
	limitFlag := flag.Int("limit", 0, "Limit number of samples to test (0 means all)")
	flag.Parse()

	npzPath := *npzPathFlag
	if npzPath == "test_data.npz" {
		npzPath = pathutils.ResolvePath("fully_homomorphic_encryption/demos/hotword/data/test_data.npz")
	}

	fmt.Printf("Loading test data from %s...\n", npzPath)
	t0 := time.Now()
	allFeatures, expectedLabels, err := loadAllTestRows(npzPath)
	if err != nil {
		fmt.Printf("Error loading test data: %v\n", err)
		os.Exit(1)
	}
	numSamples := len(allFeatures)
	if *limitFlag > 0 && *limitFlag < numSamples {
		numSamples = *limitFlag
		allFeatures = allFeatures[:numSamples]
		expectedLabels = expectedLabels[:numSamples]
	}
	fmt.Printf("  Loaded %d samples in %v\n", numSamples, time.Since(t0))

	// Configure context (ONCE)
	fmt.Println("Configuring Lattigo context...")
	t0 = time.Now()
	btpEvaluator, evaluator, params, ecd, encryptor, decryptor := hotword_lattigo.Tcresnet8small__configure()
	fmt.Printf("  Took %v\n", time.Since(t0))

	// Preprocessing (ONCE)
	fmt.Println("Running preprocessing for model weights...")
	t0 = time.Now()
	preprocessedWeights := hotword_lattigo_utils.Tcresnet8small__preprocessing(params, ecd)
	fmt.Printf("  Took %v\n", time.Since(t0))

	// 1. Sequential Encryption
	fmt.Println("Encrypting all input features and zero accumulators sequentially...")
	t0 = time.Now()
	encryptedInputs := make([][]*rlwe.Ciphertext, numSamples)
	ctZeros0 := make([]*rlwe.Ciphertext, numSamples)
	ctZeros1 := make([]*rlwe.Ciphertext, numSamples)
	ctZeros2 := make([]*rlwe.Ciphertext, numSamples)
	ctZeros3 := make([]*rlwe.Ciphertext, numSamples)
	ctZeros4 := make([]*rlwe.Ciphertext, numSamples)
	ctZeros5 := make([]*rlwe.Ciphertext, numSamples)
	ctZeros6 := make([]*rlwe.Ciphertext, numSamples)
	ctZeros7 := make([]*rlwe.Ciphertext, numSamples)
	ctZeros8 := make([]*rlwe.Ciphertext, numSamples)
	for i := 0; i < numSamples; i++ {
		encryptedInputs[i] = hotword_lattigo.Tcresnet8small__encrypt__arg0(evaluator, params, ecd, encryptor, allFeatures[i])
		ctZeros0[i] = hotword_lattigo.Tcresnet8small__encrypt__zero__0(evaluator, params, ecd, encryptor)
		ctZeros1[i] = hotword_lattigo.Tcresnet8small__encrypt__zero__1(evaluator, params, ecd, encryptor)
		ctZeros2[i] = hotword_lattigo.Tcresnet8small__encrypt__zero__2(evaluator, params, ecd, encryptor)
		ctZeros3[i] = hotword_lattigo.Tcresnet8small__encrypt__zero__3(evaluator, params, ecd, encryptor)
		ctZeros4[i] = hotword_lattigo.Tcresnet8small__encrypt__zero__4(evaluator, params, ecd, encryptor)
		ctZeros5[i] = hotword_lattigo.Tcresnet8small__encrypt__zero__5(evaluator, params, ecd, encryptor)
		ctZeros6[i] = hotword_lattigo.Tcresnet8small__encrypt__zero__6(evaluator, params, ecd, encryptor)
		ctZeros7[i] = hotword_lattigo.Tcresnet8small__encrypt__zero__7(evaluator, params, ecd, encryptor)
		ctZeros8[i] = hotword_lattigo.Tcresnet8small__encrypt__zero__8(evaluator, params, ecd, encryptor)
	}
	fmt.Printf("  Took %v\n", time.Since(t0))

	// 2. Parallel FHE Evaluation
	fmt.Println("\nStarting parallel FHE evaluation suite...")
	encryptedOutputs := make([][]*rlwe.Ciphertext, numSamples)

	var wg sync.WaitGroup
	wg.Add(numSamples)

	suiteStartTime := time.Now()
	for i := 0; i < numSamples; i++ {
		go func(idx int) {
			defer wg.Done()
			localEvaluator := evaluator.ShallowCopy()
			localBtpEvaluator := btpEvaluator.ShallowCopy()
			encryptedOutputs[idx] = hotword_lattigo.Tcresnet8small__preprocessed(
				localBtpEvaluator, localEvaluator, params, ecd, encryptedInputs[idx],
				ctZeros0[idx], ctZeros1[idx], ctZeros2[idx], ctZeros3[idx], ctZeros4[idx], ctZeros5[idx], ctZeros6[idx], ctZeros7[idx], ctZeros8[idx],
				preprocessedWeights,
			)
		}(i)
	}

	wg.Wait()
	totalEvalTime := time.Since(suiteStartTime)
	fmt.Printf("  Parallel evaluation completed in %v (average %v per sample, wall time)\n", totalEvalTime, totalEvalTime/time.Duration(numSamples))

	// 3. Sequential Decryption & Verification
	fmt.Println("\nDecrypting and verifying results sequentially...")
	correctCount := 0
	var misclassifications []struct {
		idx  int
		exp  int
		pred int
	}

	for idx := 0; idx < numSamples; idx++ {
		decryptedLogits := hotword_lattigo.Tcresnet8small__decrypt__result0(evaluator, params, ecd, decryptor, encryptedOutputs[idx])

		// Argmax
		predictedClass := 0
		maxVal := decryptedLogits[0]
		for i := 1; i < 12; i++ {
			if decryptedLogits[i] > maxVal {
				maxVal = decryptedLogits[i]
				predictedClass = i
			}
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

		fmt.Printf("Sample %3d: expected %s (%d), got %s (%d) (%s)\n",
			idx, labels[expectedLabel], expectedLabel, labels[predictedClass], predictedClass, status)
	}

	accuracy := float64(correctCount) / float64(numSamples)
	fmt.Printf("\nAccuracy: %d/%d (%.2f%%)\n", correctCount, numSamples, accuracy*100)

	if len(misclassifications) > 0 {
		fmt.Println("\nSummary of Misclassifications:")
		for _, m := range misclassifications {
			fmt.Printf("  Sample %3d: expected %s (%d), got %s (%d)\n",
				m.idx, labels[m.exp], m.exp, labels[m.pred], m.pred)
		}
	} else {
		fmt.Println("\nNO MISCLASSIFICATIONS!")
	}
}
