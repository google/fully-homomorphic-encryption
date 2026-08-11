// Evaluate a single inference
package main

import (
	"flag"
	"fmt"
	"os"
	"time"

	"fully_homomorphic_encryption/demos/common/go/pathutils"
	"fully_homomorphic_encryption/demos/hotword/lattigo/hotword_lattigo"
	"fully_homomorphic_encryption/demos/hotword/lattigo/hotword_lattigo_utils"
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
	sampleIdxFlag := flag.Int("sample_idx", 0, "Sample index in the NPZ to test")
	npzPathFlag := flag.String("npz_path", "test_data.npz", "Path to the test NPZ file")
	flag.Parse()

	npzPath := *npzPathFlag
	if npzPath == "test_data.npz" {
		npzPath = pathutils.ResolvePath("fully_homomorphic_encryption/demos/hotword/data/test_data.npz")
	}
	sampleIdx := *sampleIdxFlag

	fmt.Printf("Loading test sample %d from %s...\n", sampleIdx, npzPath)
	t0 := time.Now()
	features, expectedLabel, err := loadTestRow(npzPath, sampleIdx)
	if err != nil {
		fmt.Printf("Error loading test row: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("  Took %v\n", time.Since(t0))
	fmt.Printf("  Expected label: %s (%d)\n", labels[expectedLabel], expectedLabel)
	fmt.Printf("  Feature vector size: %d\n", len(features))

	// Configure context
	fmt.Println("Configuring Lattigo context...")
	t0 = time.Now()
	btpEvaluator, evaluator, params, ecd, encryptor, decryptor := hotword_lattigo.Tcresnet8small__configure()
	fmt.Printf("  Took %v\n", time.Since(t0))

	// Encrypt input
	fmt.Println("Encrypting input features...")
	t0 = time.Now()
	encryptedFeatures := hotword_lattigo.Tcresnet8small__encrypt__arg0(evaluator, params, ecd, encryptor, features)
	fmt.Printf("  Took %v\n", time.Since(t0))

	// Preprocessing
	fmt.Println("Running preprocessing...")
	t0 = time.Now()
	preprocessedWeights := hotword_lattigo_utils.Tcresnet8small__preprocessing(params, ecd)
	fmt.Printf("  Took %v\n", time.Since(t0))

	// FHE evaluation
	fmt.Println("Running FHE evaluation (preprocessed)...")
	t0 = time.Now()
	ctZero0 := hotword_lattigo.Tcresnet8small__encrypt__zero__0(evaluator, params, ecd, encryptor)
	ctZero1 := hotword_lattigo.Tcresnet8small__encrypt__zero__1(evaluator, params, ecd, encryptor)
	ctZero2 := hotword_lattigo.Tcresnet8small__encrypt__zero__2(evaluator, params, ecd, encryptor)
	ctZero3 := hotword_lattigo.Tcresnet8small__encrypt__zero__3(evaluator, params, ecd, encryptor)
	ctZero4 := hotword_lattigo.Tcresnet8small__encrypt__zero__4(evaluator, params, ecd, encryptor)
	ctZero5 := hotword_lattigo.Tcresnet8small__encrypt__zero__5(evaluator, params, ecd, encryptor)
	ctZero6 := hotword_lattigo.Tcresnet8small__encrypt__zero__6(evaluator, params, ecd, encryptor)
	ctZero7 := hotword_lattigo.Tcresnet8small__encrypt__zero__7(evaluator, params, ecd, encryptor)
	ctZero8 := hotword_lattigo.Tcresnet8small__encrypt__zero__8(evaluator, params, ecd, encryptor)

	encryptedOutput := hotword_lattigo.Tcresnet8small__preprocessed(
		btpEvaluator, evaluator, params, ecd, encryptedFeatures,
		ctZero0, ctZero1, ctZero2, ctZero3, ctZero4, ctZero5, ctZero6, ctZero7, ctZero8,
		preprocessedWeights,
	)
	fmt.Printf("  Took %v\n", time.Since(t0))

	// Decrypt
	fmt.Println("Decrypting output...")
	t0 = time.Now()
	decryptedLogits := hotword_lattigo.Tcresnet8small__decrypt__result0(evaluator, params, ecd, decryptor, encryptedOutput)
	fmt.Printf("  Took %v\n", time.Since(t0))

	fmt.Printf("Decrypted logits: %v\n", decryptedLogits)

	// Argmax
	predictedClass := 0
	maxVal := decryptedLogits[0]
	for i := 1; i < 12; i++ {
		if decryptedLogits[i] > maxVal {
			maxVal = decryptedLogits[i]
			predictedClass = i
		}
	}
	fmt.Printf("Predicted class: %s (%d)\n", labels[predictedClass], predictedClass)

	if predictedClass == expectedLabel {
		fmt.Println("SUCCESS: Predicted class matches expected label!")
	} else {
		fmt.Println("FAILURE: Predicted class does NOT match expected label!")
		os.Exit(1)
	}
}
