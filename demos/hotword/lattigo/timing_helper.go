// Package hotwordlattigotiming provides helper debug functions for the main routines.
package hotwordlattigotiming

import (
	"fully_homomorphic_encryption/demos/common/lattigo/debug"
	"github.com/tuneinsight/lattigo/v6/core/rlwe"
	"github.com/tuneinsight/lattigo/v6/schemes/ckks"
)

func __heir_debug(evaluator *ckks.Evaluator, param ckks.Parameters, encoder *ckks.Encoder, decryptor *rlwe.Decryptor, ctObj any, debugAttrMap map[string]string) {
	debug.HeirDebug(evaluator, param, encoder, decryptor, ctObj, debugAttrMap)
}
