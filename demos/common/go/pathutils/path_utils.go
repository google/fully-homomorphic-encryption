// Package pathutils provides shared path resolution utilities for FHE demos.
package pathutils

import (
	"fmt"
	"os"

	"github.com/bazelbuild/rules_go/go/runfiles"
)

// ResolvePath resolves a path using runfiles and verifies it exists.
// It panics if the path cannot be resolved or does not exist.
func ResolvePath(path string) string {
	absPath, err := runfiles.Rlocation(path)
	if err != nil {
		panic(fmt.Sprintf("failed to resolve path %q: %v", path, err))
	}
	if _, err := os.Stat(absPath); err != nil {
		panic(fmt.Sprintf("failed to resolve path %q: %v", path, err))
	}
	return absPath
}
