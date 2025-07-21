package(default_visibility = ["//visibility:public"])

licenses(["notice"])

exports_files(["LICENSE"])

cc_library(
    name = "libtfhe",
    hdrs = [
        "src/include/tfhe.h",
        "src/include/tfhe_io.h",
    ],
    deps = [
        ":fftw",
        ":tfhe_lib",
    ],
)

cc_library(
    name = "fftw",
    srcs = [
        "src/libtfhe/fft_processors/fftw/fft_processor_fftw.cpp",
        "src/libtfhe/fft_processors/fftw/lagrangehalfc_impl.cpp",
    ],
    hdrs = [
        "src/libtfhe/fft_processors/fftw/lagrangehalfc_impl.h",
    ],
    copts = ["-Isrc/include"],
    visibility = ["//visibility:private"],
    deps = [
        ":tfhe_lib",
        "@org_fftw//:fftw",
    ],
    alwayslink = 1,
)

cc_library(
    name = "nayuki_avx",
    srcs = [
        "src/libtfhe/fft_processors/nayuki/fft-x8664-avx.s",
        "src/libtfhe/fft_processors/nayuki/fft-x8664-avx-aux.c",
        "src/libtfhe/fft_processors/nayuki/fft-x8664-avx-reverse.s",
        "src/libtfhe/fft_processors/nayuki/fft_processor_nayuki.cpp",
        "src/libtfhe/fft_processors/nayuki/lagrangehalfc_impl.cpp",
    ],
    hdrs = [
        "src/libtfhe/fft_processors/nayuki/fft.h",
        "src/libtfhe/fft_processors/nayuki/lagrangehalfc_impl.h",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":tfhe_lib",
    ],
    alwayslink = 1,
)

cc_library(
    name = "nayuki_portable",
    srcs = [
        "src/libtfhe/fft_processors/nayuki/fft-model-of-x8664-avx.c",
        "src/libtfhe/fft_processors/nayuki/fft-x8664-avx-aux.c",
        "src/libtfhe/fft_processors/nayuki/fft_processor_nayuki.cpp",
        "src/libtfhe/fft_processors/nayuki/lagrangehalfc_impl.cpp",
    ],
    hdrs = [
        "src/libtfhe/fft_processors/nayuki/fft.h",
        "src/libtfhe/fft_processors/nayuki/lagrangehalfc_impl.h",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":tfhe_lib",
    ],
    alwayslink = 1,
)

# Note, this library contains a memory leak in its fft processor.
# Cf. https://github.com/tfhe/tfhe/issues/255
cc_library(
    name = "spqlios_avx",
    srcs = [
        "src/libtfhe/fft_processors/spqlios/fft_processor_spqlios.cpp",
        "src/libtfhe/fft_processors/spqlios/lagrangehalfc_impl.cpp",
        "src/libtfhe/fft_processors/spqlios/lagrangehalfc_impl_avx.s",
        "src/libtfhe/fft_processors/spqlios/spqlios-fft-avx.s",
        "src/libtfhe/fft_processors/spqlios/spqlios-fft-impl.cpp",
        "src/libtfhe/fft_processors/spqlios/spqlios-ifft-avx.s",
    ],
    hdrs = [
        "src/libtfhe/fft_processors/spqlios/lagrangehalfc_impl.h",
        "src/libtfhe/fft_processors/spqlios/spqlios-fft.h",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":tfhe_lib",
    ],
    alwayslink = 1,
)

cc_library(
    name = "spqlios_fma",
    srcs = [
        "src/libtfhe/fft_processors/spqlios/fft_processor_spqlios.cpp",
        "src/libtfhe/fft_processors/spqlios/lagrangehalfc_impl.cpp",
        "src/libtfhe/fft_processors/spqlios/lagrangehalfc_impl_fma.s",
        "src/libtfhe/fft_processors/spqlios/spqlios-fft-fma.s",
        "src/libtfhe/fft_processors/spqlios/spqlios-fft-impl.cpp",
        "src/libtfhe/fft_processors/spqlios/spqlios-ifft-fma.s",
    ],
    hdrs = glob(
        [
            "src/libtfhe/fft_processors/spqlios/*.h",
        ],
    ),
    visibility = ["//visibility:private"],
    deps = [
        ":tfhe_lib",
    ],
    alwayslink = 1,
)

cc_library(
    name = "tfhe_lib",
    srcs = glob(
        [
            "src/libtfhe/*.cpp",
        ],
    ),
    hdrs = glob(
        [
            "src/include/*.h",
        ],
    ),
    strip_include_prefix = "src/include",
    visibility = ["//visibility:private"],
)

cc_library(
    name = "tfhe_test_lib",
    hdrs = glob(
        [
            "src/include/*.h",
        ],
    ) + [
        "src/libtfheboot-gates.cpp",
        "src/libtfhelwe-bootstrapping-functions.cpp",
        "src/libtfhelwe-bootstrapping-functions-fft.cpp",
        "src/libtfhelwe-keyswitch-functions.cpp",
        "src/libtfhetfhe_io.cpp",
        "src/libtfhetgsw-fft-operations.cpp",
        "src/libtfhetgsw-functions.cpp",
        "src/libtfhetlwe-fft-operations.cpp",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":tfhe_lib",
    ],
)