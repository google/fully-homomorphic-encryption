package(
    default_visibility = ["//visibility:public"],
)

alias(
    name = "criteohelrm_torch.mlir",
    actual = "@criteohelrm_torch_mlir//file",
)

genrule(
    name = "copy_criteohelrm_pth",
    srcs = ["@criteohelrm_pth//file"],
    outs = ["criteohelrm.pth"],
    cmd = "cp $< $@",
)

filegroup(
    name = "criteo_data",
    srcs = [
        "sample.pt",
        ":copy_criteohelrm_pth",
    ],
)
