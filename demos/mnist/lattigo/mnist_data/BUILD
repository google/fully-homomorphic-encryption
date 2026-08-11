load("@rules_go//go:def.bzl", "go_library", "go_test")

package(default_visibility = ["//visibility:public"])

go_library(
    name = "mnist_data",
    srcs = ["mnist_data.go"],
    importpath = "fully_homomorphic_encryption/demos/mnist/lattigo/mnist_data",
)

go_test(
    name = "mnist_data_test",
    srcs = ["mnist_data_test.go"],
    data = [
        "//demos/mnist/data:mnist.npz",
    ],
    embed = [
        ":mnist_data",
    ],
)
