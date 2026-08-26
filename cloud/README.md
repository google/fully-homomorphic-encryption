# Secure GCE Deployment with Physical GPUs & HEIR FHE Compiler

A secure, high-performance, and enterprise-ready Google Cloud Platform (GCP) architecture managed via **Terraform**. This template provisions a custom VPC network, a Cloud NAT gateway, secure IAP SSH firewalls, and a dedicated **Google Compute Engine (GCE) Instance with physical NVIDIA GPU accelerators** to run **Google's HEIR (Fully Homomorphic Encryption)** compiler environment under Docker.

---

## Architecture Layout

```
┌────────────────────────────────────────────────────────┐
│                      Custom VPC                        │
│                                                        │
│  ┌──────────────────────────────────────────────────┐  │
│  │                    Subnetwork                    │  │
│  │                                                  │  │
│  │  • Private IPs only (no public exposure)         │  │
│  │  • Private Google Access enabled                 │  │
│  └─────────┬────────────────────────────────────────┘  │
│            │                                           │
│            ▼                                           │
│  ┌──────────────────────────────────────────────────┐  │
│  │             Private GCE VM (GPU)                 │  │
│  │                                                  │  │
│  │  • Physical NVIDIA GPU Accelerator               │  │
│  │  • Secure IAP Port 22 SSH Tunneling              │  │
│  │  • Outbound-only traffic via Cloud Router/NAT    │  │
│  │  • Automated startup (Docker + Toolkit + HEIR)   │  │
│  └──────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────┘
```

The infrastructure consists of:
1. **VPC Module (`modules/vpc/`):** Isolates subnets, a Cloud Router, and a Cloud NAT gateway to allow safe outbound internet access for package updates and Docker pulls without exposing public IP entrypoints. Includes a restricted port 22 firewall rule open only to **Google Identity-Aware Proxy (IAP)**.
2. **Main Configuration (`main.tf`):** Deploys the GCE GPU instance, grants specific developer IAM roles for secure IAP tunneling, and runs an automated startup script to automatically install NVIDIA drivers, Docker, the NVIDIA Container Toolkit, and launch the HEIR Docker container.
3. **Application Layer (`Dockerfile`):** Packages Google's HEIR optimizer (`heir-opt`) and translator (`heir-translate`) binaries on top of an official NVIDIA CUDA runtime image.

## Architecture Variables

<!-- BEGINNING OF PRE-COMMIT-TERRAFORM DOCS HOOK -->
## Inputs

| Name | Description | Type | Default | Required |
|------|-------------|------|---------|:--------:|
| artifact\_registry\_region | The GCP region containing the Artifact Registry repository. | `string` | `"us-central1"` | no |
| boot\_disk\_size | The boot disk size in gigabytes (GB). | `number` | `250` | no |
| boot\_disk\_type | The boot disk type (e.g. 'pd-standard', 'pd-balanced', 'pd-ssd'). | `string` | `"pd-balanced"` | no |
| boot\_image | The boot image for the VM (Ubuntu 22.04 LTS is highly recommended for Docker/NVIDIA drivers). | `string` | `"ubuntu-os-cloud/ubuntu-2204-lts"` | no |
| enable\_confidential\_vm | Set to true to enable AMD SEV/Intel TDX hardware encryption | `bool` | `false` | no |
| gpu\_count | The number of physical GPU accelerators to attach to the VM. | `number` | `1` | no |
| gpu\_type | The type of physical GPU accelerator to attach (e.g. 'nvidia-tesla-t4', 'nvidia-l4'). | `string` | `"nvidia-tesla-t4"` | no |
| image\_name | The Artifact Registry container image path (e.g., 'heir/heirimage:latest'). | `string` | `"heir/heirimage:latest"` | no |
| instance\_name | The name of the GCE confidential GPU instance. | `string` | `"l4-gpu-vm"` | no |
| machine\_type | The machine type used for the GPU VM.<br>Must be compatible with the selected GPU accelerator.<br>- For Tesla T4 (nvidia-tesla-t4): 'n1-standard-4' (or larger N1 types) is recommended.<br>- For NVIDIA L4 (nvidia-l4): 'g2-standard-4' (or larger G2 types) is recommended.<br>- For NVIDIA H100 (nvidia-h100-80gb): 'a3-highgpu-8g' is recommended. | `string` | `"n1-standard-4"` | no |
| project\_id | The Google Cloud Project ID where all resources will be deployed. | `string` | n/a | yes |
| region | The target GCP region for the custom VPC, subnetwork, router, and NAT. | `string` | `"us-west1"` | no |
| user\_email | The Google Account email of the user/developer who needs secure IAP SSH access to the VM. | `string` | `"leonardocifuentes@clsecteam.com"` | no |
| zone | The target GCP zone where the confidential GPU instance will be deployed. | `string` | `"us-west1-b"` | no |

## Outputs

| Name | Description |
|------|-------------|
| instance\_name | The name of the deployed GCE confidential GPU instance. |
| instance\_private\_ip | The internal (private) IP address of the deployed instance. |
| instance\_self\_link | The self-link of the deployed GCE confidential GPU instance. |
| instance\_zone | The zone where the instance is running. |
| subnet\_id | The ID of the custom subnet. |
| vpc\_id | The ID of the custom VPC network. |

<!-- END OF PRE-COMMIT-TERRAFORM DOCS HOOK -->


## Code Suggestions & Enhancements

During refactoring, several key architectural optimizations were implemented:
- **Modularization:** Extracted networking (VPC, subnet, router, NAT gateway, and firewall) into a dedicated `./modules/vpc` block for reusability and clean resource boundaries.
- **Complete Parameterization:** Abstracted instance names, machine types, GPUs, boot options, and credential endpoints into `variables.tf` and `terraform.tfvars`.
- **IAP IAM Gating:** Parameterized the user email (`user_email`) so that secure administrative SSH permission can be natively granted directly to the developer's identity.
- **VPC Subnet Coherence:** Aligned regions and zones between the network subnet (`us-west1`) and GCE VM (`us-west1-b`) to eliminate multi-region routing penalties.


## 🚀 How to Run and Deploy

### Prerequisites
1. **Terraform CLI** (v1.7.0+).
2. **Google Cloud SDK (gcloud)** authenticated with your GCP account.
3. Sufficient **GCP GPU Quota** in your target region (e.g. `us-west1` for a `nvidia-tesla-t4` GPU).

### Step 1: Align Your Settings
Open `terraform.tfvars.example` and edit the core parameters.

### Step 2: Initialize Terraform
Initialize the project to download providers and configure the VPC local module:
```bash
terraform init
```

### Step 3: Run a Plan and Apply
Run a speculative plan to audit what resources will be created:
```bash
terraform plan
```

If the plan looks correct, deploy the infrastructure to GCP:
```bash
terraform apply -auto-approve
```

---

## 🔍 Validation & Verification

### 1. Tunnel Securely over SSH (via IAP)
Since the VM contains no public IP address, access it securely over Identity-Aware Proxy:
```bash
gcloud compute ssh l4-gpu-vm --zone us-west1-b --tunnel-through-iap
```

### 2. Monitor GPU Installation and Docker Startup
Once inside the VM, verify the startup script is configuring the NVIDIA drivers and pulling the HEIR container:
```bash
sudo tail -f /var/log/gpu-startup.log
```
*(When completed, it will log: `GPU setup and application launch complete!`)*

### 3. Verify NVIDIA Driver Detection
Run `nvidia-smi` to confirm that the physical GPU is healthy and recognized:
```bash
nvidia-smi
```

### 4. Check Bazel tooling
Test compilation inside the active Docker container:
```bash
sudo docker exec -it my-ai-app bazel --help
```

You can run mlir files if you have it in your directory
```bash
sudo docker exec -it my-ai-app bazel run //tools:heir-opt -- \
  --mlir-to-cggi add_one_lut3.mlir \
  -o add_one_cggi.mlir
```

### 5. Execute an example
Login into the docker instance

```bash
sudo docker exec -it my-ai-app /bin/bash
```

Use Bazel to execute it
```bash
bazel run //tools:heir-opt -- \
  --mlir-to-cggi $(pwd)/tests/Examples/jaxite/add_one_lut3.mlir \
  -o $(pwd)/add_one_cggi.mlir
```
