load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "hotword_data",
    srcs = ["hotword_data.py"],
    deps = [
        requirement("librosa"),
        requirement("numpy"),
        requirement("scipy"),
        requirement("torch"),
    ],
)
