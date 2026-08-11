// Package debug provides helper debug and timing functions for Lattigo evaluations.
package debug

import (
	"fmt"
	"math"
	"time"

	"github.com/tuneinsight/lattigo/v6/core/rlwe"
	"github.com/tuneinsight/lattigo/v6/schemes/ckks"
)

// NOTE: This timing helper uses global state and is not thread-safe.
// Do not use it for parallel evaluations.
var (
	lastTime  time.Time
	startTime time.Time
	started   bool
)

// HeirDebug is the common debug helper called by HEIR-generated Lattigo code.
func HeirDebug(evaluator *ckks.Evaluator, param ckks.Parameters, encoder *ckks.Encoder, decryptor *rlwe.Decryptor, ctObj any, debugAttrMap map[string]string) {
	opName := "unknown"
	if val, ok := debugAttrMap["debug.name"]; ok && val != "" {
		opName = val
	} else if val, ok := debugAttrMap["asm.op_name"]; ok && val != "" {
		opName = val
	}

	now := time.Now()

	if !started || opName == "input" {
		startTime = now
		lastTime = now
		started = true
		fmt.Printf("[TIMING] Evaluation started at operator: %s\n", opName)
		fmt.Printf("[DEBUG] Moduli Q: %v (count: %d)\n", param.Q(), len(param.Q()))
	} else {
		sectionDuration := now.Sub(lastTime).Seconds()
		totalDuration := now.Sub(startTime).Seconds()
		fmt.Printf("[TIMING] After operator: %-16s | Section duration: %8.4f s | Total elapsed: %8.4f s\n",
			opName, sectionDuration, totalDuration)
		lastTime = now
	}

	// Print level and scale
	switch x := ctObj.(type) {
	case *rlwe.Ciphertext:
		if x != nil {
			f64 := x.Scale.Float64()
			log2Scale := math.Log2(f64)
			fmt.Printf("[DEBUG]   %s -> level: %d, scale: 2^%.2f (%v)\n", opName, x.Level(), log2Scale, f64)
		}
	case []*rlwe.Ciphertext:
		for i, ct := range x {
			if ct != nil {
				f64 := ct.Scale.Float64()
				log2Scale := math.Log2(f64)
				fmt.Printf("[DEBUG]   %s[%d] -> level: %d, scale: 2^%.2f (%v)\n", opName, i, ct.Level(), log2Scale, f64)
			}
		}
	}
}
