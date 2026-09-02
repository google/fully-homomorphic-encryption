## Description
Adds Terraform Infrastructure-as-Code (IaC) and containerization configurations to securely deploy Google's HEIR (Fully Homomorphic Encryption) compiler environment onto Google Cloud Platform (GCP) Compute Engine with physical NVIDIA GPU acceleration.

## What was changed
* **VPC Networking Module (`cloud/modules/vpc/`)**: Provisioned a custom VPC with private subnets, Cloud Router, Cloud NAT for outbound internet connectivity, and restricted Google Identity-Aware Proxy (IAP) ingress firewall rules.
* **GCE GPU Instance & IAM Provisioning (`cloud/main.tf`)**: Configured a private GCE VM supporting configurable NVIDIA GPU accelerators (Tesla T4 / L4), automated startup scripts for NVIDIA drivers and Docker container runtime, and IAM bindings for secure IAP SSH access.
* **HEIR Containerization (`cloud/Dockerfile`)**: Added a multi-stage Docker build utilizing Bazelisk and an NVIDIA CUDA runtime to build and package `heir-opt` and `heir-translate` binaries.
* **Configuration & Documentation (`cloud/README.md`, `cloud/terraform.tfvars.example`)**: Added architecture documentation, variable input/output tables, and sample configuration templates for deployment.

## How was this tested?
* [ ] Wrote new automated tests
* [ ] Ran existing test suite locally and all tests passed
* [x] Manually verified the behavior in the development environment
