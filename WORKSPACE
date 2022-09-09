""" Fully Homomorphic Encryption root worksapce """

workspace(name = "com_google_fully_homomorphic_encryption")

load(
    "@bazel_tools//tools/build_defs/repo:git.bzl",
    "new_git_repository",
)
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Install TFHE
http_archive(
    name = "rules_foreign_cc",
    sha256 = "bcd0c5f46a49b85b384906daae41d277b3dc0ff27c7c752cc51e43048a58ec83",
    strip_prefix = "rules_foreign_cc-0.7.1",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.7.1.tar.gz",
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

# Install OpenFHE

new_git_repository(
    name = "openfhe",
    build_file = "//patches:openfhe.BUILD",
    # HEAD as of 2022-04-08
    commit = "7d8e1e63e30f642ff6d8fc9493dd286b01e9c8a2",
    init_submodules = True,
    remote = "https://github.com/openfheorg/openfhe-development.git",
    shallow_since = "1649455211 -0400",
)

# Install XLS with transitive dependencies.
# 2022-06-01
http_archive(
    name = "com_google_xls",
    patches = [
        "//patches:xls-visibility-file.patch",
        "//patches:xls-visibility-ir.patch",
        "//patches:xls-visibility-logging.patch",
        "//patches:xls-visibility-status.patch",
    ],
    sha256 = "d34199f5783b444e230d6a097e1f6b1bdfbd9ffd596a1236835a3dc566b1e76d",
    strip_prefix = "xls-74933e379f651a70c1b98e146d3c4290822c743c",
    url = "https://github.com/google/xls/archive/74933e379f651a70c1b98e146d3c4290822c743c.tar.gz",
)

# Used by xlscc.
http_archive(
    name = "com_github_hlslibs_ac_types",
    build_file = "@com_google_xls//dependency_support/com_github_hlslibs_ac_types:bundled.BUILD.bazel",
    sha256 = "7ab5e2ee4c675ef6895fdd816c32349b3070dc8211b7d412242c66d0c6e8edca",
    strip_prefix = "ac_types-57d89634cb5034a241754f8f5347803213dabfca",
    urls = ["https://github.com/hlslibs/ac_types/archive/57d89634cb5034a241754f8f5347803213dabfca.tar.gz"],
)

# Install dependencies for XLS
http_archive(
    name = "com_grail_bazel_toolchain",
    sha256 = "dd03374af7885d255eb735b9065a32463a1154d9de6eb47261a49c8acc1cd497",
    strip_prefix = "bazel-toolchain-0.6.3",
    urls = [
        "https://github.com/grailbio/bazel-toolchain/archive/0.6.3.zip",
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

# Install ABC
http_archive(
    name = "abc",
    build_file = "//patches:abc.BUILD",
    sha256 = "7fa5a448a4309fb4d6cf856c3fe4cc4be46b09dd552a05d5cfacd75f8d9504ad",
    strip_prefix = "abc-eb44a80bf2eb8723231e72bb095c97d1e4834d56",
    urls = [
        "https://github.com/berkeley-abc/abc/archive/eb44a80bf2eb8723231e72bb095c97d1e4834d56.zip",
    ],
)

# Install Yosys
http_archive(
    name = "yosys",
    build_file = "//patches:yosys.BUILD",
    patch_args = ["-p1"],
    patches = ["//patches:0001-Fetch-YOSYS_DATDIR-from-the-environment.patch"],
    sha256 = "b81994853fb4a12b45afc6503568e84499025add8e51615c24b164cf2e8359a8",
    strip_prefix = "yosys-yosys-0.20",
    urls = [
        "https://github.com/YosysHQ/yosys/archive/refs/tags/yosys-0.20.zip",
    ],
)

load("@com_google_xls//dependency_support:initialize_external.bzl", "initialize_external_repositories")

initialize_external_repositories()
