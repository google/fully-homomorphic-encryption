package main

import (
	"flag"
	"fmt"
	"math"
	"os"
	"path/filepath"
	"time"

	"fully_homomorphic_encryption/demos/common/go/pathutils"
	"fully_homomorphic_encryption/demos/mnist/lattigo/mnist"
	"fully_homomorphic_encryption/demos/mnist/lattigo/mnist_data"
	"fully_homomorphic_encryption/demos/mnist/lattigo/mnist_utils"
)

var (
	numSamples = flag.Int("num_samples", 10, "Number of test samples to evaluate sequentially (default 10)")
	dataDir    = flag.String("data_dir", "fully_homomorphic_encryption/demos/mnist/data", "Directory containing MNIST dataset binary files")
)

func main() {
	flag.Parse()

	fmt.Println("Configuring Lattigo crypto context...")
	tSetupStart := time.Now()
	evaluator, params, encoder, encryptor, decryptor := mnist.Mnist__configure()
	tSetupEnd := time.Now()
	fmt.Printf("Crypto context setup completed in %.2f ms.\n\n", float64(tSetupEnd.Sub(tSetupStart).Microseconds())/1000.0)

	fmt.Println("Preprocessing weights...")
	tPreStart := time.Now()
	preprocessedWeights := mnist_utils.Mnist__preprocessing(params, encoder)
	tPreEnd := time.Now()
	fmt.Printf("Weight preprocessing completed in %.2f ms.\n\n", float64(tPreEnd.Sub(tPreStart).Microseconds())/1000.0)

	fmt.Println("Encrypting zero constant vectors...")
	ctZeros0 := mnist.Mnist__encrypt__zero__0(evaluator, params, encoder, encryptor)
	ctZeros1 := mnist.Mnist__encrypt__zero__1(evaluator, params, encoder, encryptor)
	ctZeros2 := mnist.Mnist__encrypt__zero__2(evaluator, params, encoder, encryptor)

	fmt.Printf("Evaluating %d MNIST samples sequentially...\n", *numSamples)
	correct := 0
	var totalEvalDuration time.Duration

	npzPath := pathutils.ResolvePath(filepath.Join(*dataDir, "mnist.npz"))
	images, labels, err := mnist_data.LoadMNISTNPZ(npzPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error loading MNIST dataset: %v\n", err)
		os.Exit(1)
	}
	if *numSamples > len(images) {
		fmt.Fprintf(os.Stderr, "Requested %d samples, but only %d available\n", *numSamples, len(images))
		os.Exit(1)
	}

	for i := 0; i < *numSamples; i++ {
		image := images[i]
		label := labels[i]

		ctInput := mnist.Mnist__encrypt__arg0(evaluator, params, encoder, encryptor, image)

		tEvalStart := time.Now()
		resCt := mnist.Mnist__preprocessed(evaluator, params, encoder, ctInput, ctZeros0, ctZeros1, ctZeros2, preprocessedWeights)
		evalDuration := time.Since(tEvalStart)
		totalEvalDuration += evalDuration

		resValues := mnist.Mnist__decrypt__result0(evaluator, params, encoder, decryptor, resCt)

		maxVal := float32(-math.MaxFloat32)
		pred := -1
		for j := 0; j < 10 && j < len(resValues); j++ {
			if resValues[j] > maxVal {
				maxVal = resValues[j]
				pred = j
			}
		}

		isCorrect := pred == label
		if isCorrect {
			correct++
		}

		status := "INCORRECT"
		if isCorrect {
			status = "CORRECT"
		}
		fmt.Printf("Sample %4d: True=%d, Pred=%d | %s | Eval Time= %.2f ms\n", i, label, pred, status, float64(evalDuration.Microseconds())/1000.0)
	}

	accuracy := 0.0
	avgEvalMs := 0.0
	if *numSamples > 0 {
		accuracy = (float64(correct) / float64(*numSamples)) * 100.0
		avgEvalMs = float64(totalEvalDuration.Microseconds()) / (1000.0 * float64(*numSamples))
	}

	fmt.Println("\n--- Lattigo Evaluation Suite Results ---")
	fmt.Printf("Total Samples Evaluated: %d\n", *numSamples)
	fmt.Printf("Total Correct:           %d / %d\n", correct, *numSamples)
	fmt.Printf("Overall Accuracy:        %.2f%%\n", accuracy)
	fmt.Printf("Average Homomorphic Eval Latency: %.2f ms/sample\n", avgEvalMs)
}
