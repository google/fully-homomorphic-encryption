package(default_visibility = ["//visibility:public"])

exports_files([
    "Mirai_first_batch_32K.bin",
    "Mirai_full_50_features_32K.bin",
    "Mirai_labels.csv",
    "torch_50_kitnet_model.pt",
    "torch_50_kitnet_model_annotated.mlir",
    "torch_kitnet_model.pt",
    "torch_kitnet_model_annotated.mlir",
    "torch_kitnet_model_timing.mlir",
])

filegroup(
    name = "data",
    srcs = glob([
        "*.bin",
        "*.csv",
        "*.mlir",
        "*.pt",
    ]),
)
