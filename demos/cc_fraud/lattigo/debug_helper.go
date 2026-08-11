package fraud_model_lattigo_debug

import (
	"encoding/json"
	"fmt"
	"math"
	"os"
	"sort"
	"strconv"
	"sync"

	"fully_homomorphic_encryption/demos/common/go/pathutils"
	"github.com/tuneinsight/lattigo/v6/core/rlwe"
	"github.com/tuneinsight/lattigo/v6/schemes/ckks"
)

var (
	refData     map[string]map[string][]float64
	loadOnce    sync.Once
	loadErr     error
	debugRowIdx int = -1
)

func loadReferenceData() {
	path := pathutils.ResolvePath("fully_homomorphic_encryption/demos/cc_fraud/debug/debug_reference.json")
	var file *os.File
	file, loadErr = os.Open(path)
	if loadErr != nil {
		return
	}
	defer file.Close()

	decoder := json.NewDecoder(file)
	loadErr = decoder.Decode(&refData)
}

func getRowIdx() int {
	if debugRowIdx != -1 {
		return debugRowIdx
	}

	rowStr := os.Getenv("HEIR_DEBUG_ROW_IDX")
	if rowStr == "" {
		// Fallback to reading from a temp file or just default to 0
		// In Go, it might be harder to pass flags to the helper, so env var is best.
		rowStr = "0"
	}

	idx, err := strconv.Atoi(rowStr)
	if err != nil {
		fmt.Printf("  [DEBUG] Invalid HEIR_DEBUG_ROW_IDX '%s', defaulting to 0\n", rowStr)
		debugRowIdx = 0
	} else {
		debugRowIdx = idx
	}
	return debugRowIdx
}

var allowedSteps = map[string]bool{
	"input":          true,
	"layer1_matmul":  true,
	"layer1_bias":    true,
	"layer1_sigmoid": true,
	"layer2_matmul":  true,
	"layer2_bias":    true,
	"layer2_sigmoid": true,
	"layer3_matmul":  true,
	"layer3_bias":    true,
}

func __heir_debug(evaluator *ckks.Evaluator, param ckks.Parameters, encoder *ckks.Encoder, decryptor *rlwe.Decryptor, ctObj any, debugAttrMap map[string]string) {
	loadOnce.Do(loadReferenceData)

	var ct *rlwe.Ciphertext
	switch v := ctObj.(type) {
	case *rlwe.Ciphertext:
		ct = v
	case []*rlwe.Ciphertext:
		if len(v) == 0 {
			fmt.Println("  [DEBUG] Empty ciphertext slice")
			return
		}
		ct = v[0]
		if ct == nil {
			fmt.Println("  [DEBUG] First ciphertext element is nil")
			return
		}
	default:
		panic(fmt.Sprintf("unexpected type %T", ctObj))
	}

	stepName := debugAttrMap["debug.name"]
	if !allowedSteps[stepName] {
		return
	}

	rowIdx := getRowIdx()
	rowKey := fmt.Sprintf("row_%d", rowIdx)

	// Determine size to decode
	messageSizeStr, ok := debugAttrMap["message.size"]
	var messageSize int
	var err error
	if !ok || messageSizeStr == "" {
		messageSize = 1
	} else {
		messageSize, err = strconv.Atoi(messageSizeStr)
		if err != nil {
			messageSize = 1
		}
	}

	// Decrypt and decode
	pt := decryptor.DecryptNew(ct)
	values := make([]float64, param.MaxSlots())
	encoder.Decode(pt, values)

	// Slice to actual size
	fheVals := values[:messageSize]

	// Print FHE values
	printSize := 5
	if len(fheVals) < printSize {
		printSize = len(fheVals)
	}
	fmt.Printf("[DEBUG] Step: %s (row_%d)\n", stepName, rowIdx)
	fmt.Printf("  FHE Decrypted (first min(5, size)): %v (size: %d)\n", fheVals[:printSize], len(fheVals))
	fmt.Printf("  Scale: 2^%3.3f\n", ct.Scale.Log2())

	if loadErr != nil {
		fmt.Printf("  [WARNING] Failed to load reference data: %v\n", loadErr)
		return
	}

	// Compare with reference
	if rowData, ok := refData[rowKey]; ok {
		if refVals, ok := rowData[stepName]; ok {
			refPrintSize := 5
			if len(refVals) < refPrintSize {
				refPrintSize = len(refVals)
			}
			fmt.Printf("  Expected Ref  (first min(5, size)): %v\n", refVals[:refPrintSize])

			// Slice FHE values to match reference size
			compareFheVals := fheVals
			if len(fheVals) > len(refVals) {
				compareFheVals = fheVals[:len(refVals)]
			}

			// Calculate max absolute error
			maxAbsErr := 0.0
			for i := 0; i < len(compareFheVals); i++ {
				err := math.Abs(compareFheVals[i] - refVals[i])
				if err > maxAbsErr {
					maxAbsErr = err
				}
			}
			fmt.Printf("  Max Abs Error: %e\n", maxAbsErr)
			if maxAbsErr > 0.0 {
				fmt.Printf("  Precision Lost: 2^%3.3f bits\n", math.Log2(maxAbsErr))
			} else {
				fmt.Println("  Precision Lost: 0 bits (exact)")
			}

			// Sorted Check
			if len(compareFheVals) == len(refVals) {
				sortedFhe := make([]float64, len(compareFheVals))
				copy(sortedFhe, compareFheVals)
				sortedRef := make([]float64, len(refVals))
				copy(sortedRef, refVals)

				sort.Float64s(sortedFhe)
				sort.Float64s(sortedRef)

				maxSortedErr := 0.0
				for i := 0; i < len(sortedFhe); i++ {
					err := math.Abs(sortedFhe[i] - sortedRef[i])
					if err > maxSortedErr {
						maxSortedErr = err
					}
				}
				fmt.Printf("  [Sorted Check] Max Abs Error: %e\n", maxSortedErr)
				if maxSortedErr > 0.0 {
					fmt.Printf("  [Sorted Check] Precision Lost: 2^%3.3f bits\n", math.Log2(maxSortedErr))
				} else {
					fmt.Println("  [Sorted Check] Precision Lost: 0 bits (exact)")
				}
			} else {
				fmt.Printf("  [Sorted Check] Skip (size mismatch: FHE %d vs Ref %d)\n", len(compareFheVals), len(refVals))
			}

		} else {
			fmt.Printf("  [WARNING] No reference data found for step '%s'\n", stepName)
		}
	} else {
		fmt.Printf("  [WARNING] No reference data found for row '%s'\n", rowKey)
	}
}
