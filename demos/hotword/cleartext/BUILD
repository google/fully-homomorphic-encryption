load("@demo_pip_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_binary")

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "evaluate_cleartext",
    srcs = ["evaluate_cleartext.py"],
    data = [
        "//demos/hotword/data:tc_resnet8.pth",
        "//demos/hotword/data:tcresnet8-small.pth",
        "//demos/hotword/data:test_data.npz",
        "//demos/hotword/data:test_data-small.npz",
    ],
    main = "evaluate_cleartext.py",
    deps = [
        "//demos/common/python:path_utils",
        "//demos/hotword/torch:model",
        "//demos/hotword/utils:hotword_data",
        requirement("torch"),
    ],
)

py_binary(
    name = "evaluate_cleartext_suite",
    srcs = ["evaluate_cleartext_suite.py"],
    data = [
        "//demos/hotword/data:tc_resnet8.pth",
        "//demos/hotword/data:tcresnet8-small.pth",
        "//demos/hotword/data:test_data.npz",
        "//demos/hotword/data:test_data-small.npz",
    ],
    main = "evaluate_cleartext_suite.py",
    deps = [
        "//demos/common/python:path_utils",
        "//demos/hotword/torch:model",
        "//demos/hotword/utils:hotword_data",
        requirement("torch"),
    ],
)

py_binary(
    name = "calibrate",
    srcs = ["calibrate.py"],
    data = [
        "//demos/hotword/data:tc_resnet8.pth",
        "//demos/hotword/data:tcresnet8-small.pth",
        "//demos/hotword/data:test_data.npz",
        "//demos/hotword/data:test_data-small.npz",
    ],
    main = "calibrate.py",
    deps = [
        "//demos/common/python:path_utils",
        "//demos/hotword/torch:model",
        "//demos/hotword/utils:hotword_data",
        requirement("torch"),
    ],
)
