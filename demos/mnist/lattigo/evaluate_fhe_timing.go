// Evaluates FHE inference with timing helpers
package main

import (
	"flag"
	"fmt"
	"math"
	"os"
	"path/filepath"
	"time"

	"fully_homomorphic_encryption/demos/common/go/pathutils"
	"fully_homomorphic_encryption/demos/mnist/lattigo/mnist_data"
	"fully_homomorphic_encryption/demos/mnist/lattigo/mnist_timing"
	"fully_homomorphic_encryption/demos/mnist/lattigo/mnist_timing_utils"
)

var (
	sampleIdx = flag.Int("sample_idx", 0, "Index of the MNIST sample to evaluate (0-9999)")
	dataDir   = flag.String("data_dir", "fully_homomorphic_encryption/demos/mnist/data", "Directory containing MNIST dataset binary files")
)

func main() {
	flag.Parse()

	t0 := time.Now()
	npzPath := pathutils.ResolvePath(filepath.Join(*dataDir, "mnist.npz"))
	image, label, err := mnist_data.LoadMNISTSampleNPZ(npzPath, *sampleIdx)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error loading sample %d: %v\n", *sampleIdx, err)
		os.Exit(1)
	}

	fmt.Printf("\nEvaluating MNIST Sample Index (Timing Variant): %d\n", *sampleIdx)
	fmt.Println("--- Step Latencies ---")

	tSetupStart := time.Now()
	evaluator, params, encoder, encryptor, decryptor := mnist_timing.Mnist__configure()
	setupDuration := time.Since(tSetupStart)
	fmt.Printf("Crypto Setup & KeyGen:   %.4f ms\n", float64(setupDuration.Microseconds())/1000.0)

	tPreStart := time.Now()
	preprocessedWeights := mnist_timing_utils.Mnist__preprocessing(params, encoder)
	preprocessDuration := time.Since(tPreStart)
	fmt.Printf("Weight Preprocessing:    %.4f ms\n", float64(preprocessDuration.Microseconds())/1000.0)

	tEncStart := time.Now()
	ctInput := mnist_timing.Mnist__encrypt__arg0(evaluator, params, encoder, encryptor, image)
	ctZeros0 := mnist_timing.Mnist__encrypt__zero__0(evaluator, params, encoder, encryptor)
	ctZeros1 := mnist_timing.Mnist__encrypt__zero__1(evaluator, params, encoder, encryptor)
	ctZeros2 := mnist_timing.Mnist__encrypt__zero__2(evaluator, params, encoder, encryptor)
	encryptDuration := time.Since(tEncStart)
	fmt.Printf("Input & Zero Encryption: %.4f ms\n", float64(encryptDuration.Microseconds())/1000.0)

	tEvalStart := time.Now()
	// Pass decryptor for intermediate timing/debug callbacks
	resCt := mnist_timing.Mnist__preprocessed(evaluator, params, encoder, decryptor, ctInput, ctZeros0, ctZeros1, ctZeros2, preprocessedWeights)
	evalDuration := time.Since(tEvalStart)
	fmt.Printf("Homomorphic Evaluation:  %.4f ms\n", float64(evalDuration.Microseconds())/1000.0)

	tDecStart := time.Now()
	resValues := mnist_timing.Mnist__decrypt__result0(evaluator, params, encoder, decryptor, resCt)
	decryptDuration := time.Since(tDecStart)
	fmt.Printf("Decryption:              %.4f ms\n", float64(decryptDuration.Microseconds())/1000.0)

	totalDuration := time.Since(t0)
	fmt.Printf("Total Latency:           %.4f ms\n", float64(totalDuration.Microseconds())/1000.0)
	fmt.Println("-------------------------------")

	maxVal := float32(-math.MaxFloat32)
	pred := -1
	for j := 0; j < 10 && j < len(resValues); j++ {
		if resValues[j] > maxVal {
			maxVal = resValues[j]
			pred = j
		}
	}

	fmt.Printf("True Label:      %d\n", label)
	fmt.Printf("Predicted Label: %d\n", pred)
	if pred == label {
		fmt.Println("Result:          CORRECT")
	} else {
		fmt.Println("Result:          INCORRECT")
	}
}
