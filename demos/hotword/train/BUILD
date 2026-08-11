load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "train_tc_resnet",
    srcs = ["train_tc_resnet.py"],
    main = "train_tc_resnet.py",
    deps = [
        "//demos/hotword/torch:model",
        "//demos/hotword/utils:hotword_data",
        requirement("numpy"),
        requirement("torch"),
    ],
)
