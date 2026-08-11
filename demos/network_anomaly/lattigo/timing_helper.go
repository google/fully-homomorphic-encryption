// Package anomaly_model_lattigo_timing provides timing and debug helper functions for Lattigo evaluation.
package anomaly_model_lattigo_timing

import (
	"fully_homomorphic_encryption/demos/common/lattigo/debug"
	"github.com/tuneinsight/lattigo/v6/core/rlwe"
	"github.com/tuneinsight/lattigo/v6/schemes/ckks"
)

func __heir_debug(evaluator *ckks.Evaluator, param ckks.Parameters, encoder *ckks.Encoder, decryptor *rlwe.Decryptor, ctObj any, debugAttrMap map[string]string) {
	debug.HeirDebug(evaluator, param, encoder, decryptor, ctObj, debugAttrMap)
}
