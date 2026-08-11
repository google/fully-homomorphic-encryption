package mnist_data

import (
	"os"
	"path/filepath"
	"testing"
)

func resolvePath(path string) string {
	if _, err := os.Stat(path); err == nil {
		return path
	}
	for _, env := range []string{"TEST_SRCDIR", "RUNFILES_DIR"} {
		if root := os.Getenv(env); root != "" {
			for _, prefix := range []string{"fully_homomorphic_encryption", "_main", ""} {
				candidate := filepath.Join(root, prefix, path)
				if _, err := os.Stat(candidate); err == nil {
					return candidate
				}
			}
		}
	}
	return path
}

func TestLoadMNISTNPZ(t *testing.T) {
	npzPath := resolvePath("demos/mnist/data/mnist.npz")
	if _, err := os.Stat(npzPath); os.IsNotExist(err) {
		t.Fatalf("mnist.npz not found at %s", npzPath)
	}

	images, labels, err := LoadMNISTNPZ(npzPath)
	if err != nil {
		t.Fatalf("LoadMNISTNPZ failed: %v", err)
	}

	if len(images) != 10000 {
		t.Errorf("expected 10000 images, got %d", len(images))
	}
	if len(labels) != 10000 {
		t.Errorf("expected 10000 labels, got %d", len(labels))
	}

	if len(images) > 0 {
		if len(images[0]) != 784 {
			t.Errorf("expected image size 784, got %d", len(images[0]))
		}
		// Check some pixel values are normalized
		// MNIST pixels are 0-255.
		// 0 normalized: (0/255 - 0.1307)/0.3081 = -0.4242
		// 255 normalized: (1 - 0.1307)/0.3081 = 2.8214
		// We expect values to be roughly in this range.
		for _, val := range images[0] {
			if val < -0.5 || val > 3.0 {
				t.Errorf("unexpected normalized pixel value: %f", val)
			}
		}
	}
}

func TestLoadMNISTSampleNPZ(t *testing.T) {
	npzPath := resolvePath("demos/mnist/data/mnist.npz")

	image, label, err := LoadMNISTSampleNPZ(npzPath, 0)
	if err != nil {
		t.Fatalf("LoadMNISTSampleNPZ failed: %v", err)
	}

	if len(image) != 784 {
		t.Errorf("expected image size 784, got %d", len(image))
	}

	// Sample 0 in MNIST test set is typically a 7.
	// Let's verify if the label matches.
	// Note: We need to verify if the copied npz actually has the same order.
	// If it is tensorflow_io test_mnist, it should be the standard MNIST test set.
	// Standard MNIST test sample 0 is indeed 7.
	if label != 7 {
		t.Errorf("expected label 7 for sample 0, got %d", label)
	}
}
