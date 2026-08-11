// Runs a single inference of the HE-LRM model in FHE
package main

import (
	"fmt"
	"time"

	"fully_homomorphic_encryption/demos/criteo/lattigo/criteo"
	"fully_homomorphic_encryption/demos/criteo/lattigo/criteo_utils"
)

func main() {
	fmt.Println("Generating synthetic inputs...")
	// Input sizes determined from generated code analysis:
	// arg0: dense features (size 13)
	// arg1: sparse features 1 (size 23873)
	// arg2: sparse features 2 (size 23873)
	input0 := make([]float32, 13)
	for i := range input0 {
		input0[i] = 1.0
	}
	input1 := make([]float32, 23873)
	for i := range input1 {
		input1[i] = 0.5
	}
	input2 := make([]float32, 23873)
	for i := range input2 {
		input2[i] = 0.2
	}

	fmt.Println("Configuring Lattigo context...")
	t0 := time.Now()
	bootstrappingEvaluator, evaluator, params, encoder, encryptor, decryptor := criteo.Run_inference__configure()
	fmt.Printf("  Took %v\n", time.Since(t0))

	fmt.Println("Encrypting inputs...")
	t0 = time.Now()
	cts0 := criteo.Run_inference__encrypt__arg0(evaluator, params, encoder, encryptor, input0)
	cts1 := criteo.Run_inference__encrypt__arg1(evaluator, params, encoder, encryptor, input1)
	cts2 := criteo.Run_inference__encrypt__arg2(evaluator, params, encoder, encryptor, input2)
	fmt.Printf("  Took %v\n", time.Since(t0))

	fmt.Println("Encrypting zeros...")
	t0 = time.Now()
	zero0 := criteo.Run_inference__encrypt__zero__0(evaluator, params, encoder, encryptor)
	zero1 := criteo.Run_inference__encrypt__zero__1(evaluator, params, encoder, encryptor)
	zero2 := criteo.Run_inference__encrypt__zero__2(evaluator, params, encoder, encryptor)
	zero3 := criteo.Run_inference__encrypt__zero__3(evaluator, params, encoder, encryptor)
	zero4 := criteo.Run_inference__encrypt__zero__4(evaluator, params, encoder, encryptor)
	zero5 := criteo.Run_inference__encrypt__zero__5(evaluator, params, encoder, encryptor)
	zero6 := criteo.Run_inference__encrypt__zero__6(evaluator, params, encoder, encryptor)
	fmt.Printf("  Took %v\n", time.Since(t0))

	fmt.Println("Running preprocessing...")
	t0 = time.Now()
	preprocessedWeights := criteo_utils.Run_inference__preprocessing(params, encoder)
	fmt.Printf("  Took %v\n", time.Since(t0))

	fmt.Println("Running FHE evaluation (preprocessed)...")
	t0 = time.Now()
	encryptedOutput := criteo.Run_inference__preprocessed(
		bootstrappingEvaluator, evaluator, params, encoder,
		cts0, cts1, cts2,
		zero0, zero1, zero2, zero3, zero4, zero5, zero6,
		preprocessedWeights,
	)
	fmt.Printf("  Took %v\n", time.Since(t0))

	fmt.Println("Decrypting output...")
	t0 = time.Now()
	output := criteo.Run_inference__decrypt__result0(evaluator, params, encoder, decryptor, encryptedOutput)
	fmt.Printf("  Took %v\n", time.Since(t0))

	fmt.Printf("Output logit: %v\n", output)
}
