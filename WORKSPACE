""" Fully Homomorphic Encryption root workspace """

workspace(name = "com_google_fully_homomorphic_encryption")

load(
    "@bazel_tools//tools/build_defs/repo:git.bzl",
    "new_git_repository",
)
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# provides the `license` rule, which is required because gentbl_rule implicitly
# depends upon the target '//:license'.
http_archive(
    name = "rules_license",
    sha256 = "6157e1e68378532d0241ecd15d3c45f6e5cfd98fc10846045509fb2a7cc9e381",
    urls = [
        "https://github.com/bazelbuild/rules_license/releases/download/0.0.4/rules_license-0.0.4.tar.gz",
    ],
)

http_archive(
    name = "bazel_skylib",
    sha256 = "74d544d96f4a5bb630d465ca8bbcfe231e3594e5aae57e1edbf17a6eb3ca2506",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
    ],
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

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
    name = "com_nlohmann_json",
    build_file = "//patches:nlohmann_json.BUILD",
    sha256 = "d69f9deb6a75e2580465c6c4c5111b89c4dc2fa94e3a85fcd2ffcd9a143d9273",
    strip_prefix = "json-3.11.2",
    url = "https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.tar.gz",
)

http_archive(
    name = "tfhe",
    build_file = "//patches:tfhe.BUILD",
    sha256 = "7ad88b70b389bfdb871488a90372b0cecd9ba731183ba02c3cd0ce86c9adcc93",
    strip_prefix = "tfhe-a085efe91314f994285fcb06ab8bdae3d55e4505",
    url = "https://github.com/tfhe/tfhe/archive/a085efe91314f994285fcb06ab8bdae3d55e4505.tar.gz",
)

# Install OpenFHE
# git_repository is required because the git project uses submodules,
# (see .gitmodules in the source project), and hence we have to rely
# on init_submodules to get proper behavior
new_git_repository(
    name = "openfhe",
    build_file = "//patches:openfhe.BUILD",
    # v1.0.0, 2022-11-03
    commit = "5fc1a84cad234ffdea547db03985d888ff943ad1",
    init_submodules = True,
    remote = "https://github.com/openfheorg/openfhe-development.git",
    shallow_since = "1667502783 +0200",
)

# Install XLS and its transitive dependencies.
http_archive(
    name = "com_google_xls",
    sha256 = "3c7a0d7201a6c87cff274a2bd1a2c88ae2b15dffefd9053936e8000ec0cba106",
    strip_prefix = "xls-1da08a2e3146a0708719a273119f24d56a8e2015",
    url = "https://github.com/google/xls/archive/1da08a2e3146a0708719a273119f24d56a8e2015.tar.gz",
)

# Used by xlscc.
http_archive(
    name = "com_github_hlslibs_ac_types",
    build_file = "@com_google_xls//dependency_support/com_github_hlslibs_ac_types:bundled.BUILD.bazel",
    sha256 = "7ab5e2ee4c675ef6895fdd816c32349b3070dc8211b7d412242c66d0c6e8edca",
    strip_prefix = "ac_types-57d89634cb5034a241754f8f5347803213dabfca",
    urls = ["https://github.com/hlslibs/ac_types/archive/57d89634cb5034a241754f8f5347803213dabfca.tar.gz"],
)

# Toolchain to install LLVM, a requirements for XLS
http_archive(
    name = "com_grail_bazel_toolchain",
    sha256 = "06e1421091f153029c070f1ae364f8cb5a61dab20ede97a844a0f7bfcec632a4",
    strip_prefix = "bazel-toolchain-0.8",
    urls = [
        "https://github.com/grailbio/bazel-toolchain/archive/refs/tags/0.8.zip",
    ],
)

load("@com_grail_bazel_toolchain//toolchain:deps.bzl", "bazel_toolchain_dependencies")

bazel_toolchain_dependencies()

load("@com_grail_bazel_toolchain//toolchain:rules.bzl", "llvm_toolchain")

llvm_toolchain(
    name = "llvm_toolchain",
    llvm_version = "14.0.0",
    strip_prefix = {"": "clang+llvm-14.0.0-x86_64-linux-gnu-ubuntu-18.04"},
    # In Docker, this ensures that we use the toolchain uses a pre-installed LLVM,
    # which is required because llvm_toolchain does not support Docker using its
    # introspection capabilities.
    urls = {
        "": [
            "https://github.com/llvm/llvm-project/releases/download/llvmorg-14.0.0/clang+llvm-14.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz",
        ],
    },
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

# Install Yosys. Using git rule because Yosys archive checksum keeps changing
# and they can't figure out why see
# https://github.com/YosysHQ/yosys/issues/3479
new_git_repository(
    name = "yosys",
    build_file = "//patches:yosys.BUILD",
    # Release v0.21
    commit = "e6d2a900a979df59bee82a6293e467411a0bac7c",
    remote = "https://github.com/YosysHQ/yosys.git",
    shallow_since = "1662445410 +0200",
)

# Install google benchmark framework
new_git_repository(
    name = "com_google_benchmark",
    build_file = "//patches:google_benchmark.BUILD",
    commit = "d572f4777349d43653b21d6c2fc63020ab326db2",
    remote = "https://github.com/google/benchmark.git",
)

load("@com_google_xls//dependency_support:initialize_external.bzl", "initialize_external_repositories")

initialize_external_repositories()

# Install rules_rust for rust codegen
http_archive(
    name = "rules_rust",
    sha256 = "4a9cb4fda6ccd5b5ec393b2e944822a62e050c7c06f1ea41607f14c4fdec57a2",
    urls = ["https://github.com/bazelbuild/rules_rust/releases/download/0.25.1/rules_rust-v0.25.1.tar.gz"],
)

load("@rules_rust//rust:repositories.bzl", "rules_rust_dependencies", "rust_register_toolchains")

rules_rust_dependencies()

rust_register_toolchains(
    edition = "2021",
    versions = [
        # see https://github.com/bazelbuild/rules_rust/blob/main/util/fetch_shas_VERSIONS.txt
        "1.70.0",
    ],
)

# Rust dependencies ("Cargo free" setup)
load("@rules_rust//crate_universe:defs.bzl", "crate", "crates_repository", "render_config")

crates_repository(
    name = "crate_index",
    cargo_lockfile = "//:Cargo.lock",
    lockfile = "//:Cargo.Bazel.lock",
    packages = {
        "tfhe": crate.spec(
            features = [
                "boolean",
                "shortint",
                "x86_64-unix",
            ],
            version = "0.2.4",
        ),
        "rayon": crate.spec(
            version = "1.7.0",
        ),
    },
    render_config = render_config(
        default_package_name = "",
    ),
)

load("@crate_index//:defs.bzl", "crate_repositories")

crate_repositories()

# @xls_pip_deps installs rules_python and uses the system python, and for
# whatever reason that means we can't use a hermetic python here. If you try,
# you will see it fail to load numpy C-extensions.
load("@rules_python//python:pip.bzl", "pip_install")

pip_install(
    name = "transpiler_pip_deps",
    python_interpreter = "python3",
    requirements = "//:requirements.txt",
)
