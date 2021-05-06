""" Fully Homomorphic Encryption root worksapce """

workspace(name = "com_google_fully_homomorphic_encryption")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Install TFHE
http_archive(
    name = "rules_foreign_cc",
    sha256 = "e60cfd0a8426fa4f5fd2156e768493ca62b87d125cb35e94c44e79a3f0d8635f",
    strip_prefix = "rules_foreign_cc-0.2.0",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.2.0.zip",
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

http_archive(
    name = "tfhe",
    build_file = "//patches:tfhe.BUILD",
    sha256 = "7ad88b70b389bfdb871488a90372b0cecd9ba731183ba02c3cd0ce86c9adcc93",
    strip_prefix = "tfhe-a085efe91314f994285fcb06ab8bdae3d55e4505",
    url = "https://github.com/tfhe/tfhe/archive/a085efe91314f994285fcb06ab8bdae3d55e4505.tar.gz",
)

# Install XLS with transitive dependencies.
# 2021-05-03
http_archive(
    name = "com_google_xls",
    patches = [
        "//patches:xls-visibility-file.patch",
        "//patches:xls-visibility-ir.patch",
        "//patches:xls-visibility-logging.patch",
        "//patches:xls-visibility-status.patch",
    ],
    sha256 = "39a8eb66caa203602c8332c9f61d8f5d6c99865862d272d52e0485711c2c59ab",
    strip_prefix = "xls-f4f1b0c5a290ad1df8b896732a1a6ee02a452a3e",
    url = "https://github.com/google/xls/archive/f4f1b0c5a290ad1df8b896732a1a6ee02a452a3e.tar.gz",
)

# Install dependencies for XLS
http_archive(
    name = "com_grail_bazel_toolchain",
    patches = ["@com_google_xls//dependency_support/com_grail_bazel_toolchain:google_workstation_workaround.patch"],
    sha256 = "0246482b21a04667825c655d3b4f8f796d842817b2e11f536bbfed5673cbfd97",
    strip_prefix = "bazel-toolchain-f2d1ba2c9d713b2aa6e7063f6d11dd3d64aa402a",
    urls = [
        "https://github.com/grailbio/bazel-toolchain/archive/f2d1ba2c9d713b2aa6e7063f6d11dd3d64aa402a.zip",
    ],
)

load("@com_grail_bazel_toolchain//toolchain:deps.bzl", "bazel_toolchain_dependencies")

bazel_toolchain_dependencies()

load("@com_grail_bazel_toolchain//toolchain:rules.bzl", "llvm_toolchain")

llvm_toolchain(
    name = "llvm_toolchain",
    # Try using llvm version 10.0.1 if you are getting an "Unknown LLVM
    # release" error.  This is a known issue for Ubuntu 16.04 LTS.
    llvm_version = "10.0.0",
)

load("@llvm_toolchain//:toolchains.bzl", "llvm_register_toolchains")

llvm_register_toolchains()

load("@com_google_xls//dependency_support:load_external.bzl", "load_external_repositories")

load_external_repositories()

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

load("@com_google_xls//dependency_support:initialize_external.bzl", "initialize_external_repositories")

initialize_external_repositories()
