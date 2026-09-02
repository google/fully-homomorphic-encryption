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


### Required Google Cloud APIs

Before provisioning the infrastructure, enable all required Google Cloud APIs for your project:

| API Service Name | Service Identifier | Purpose / Description |
|---|---|---|
| **Compute Engine API** | `compute.googleapis.com` | Provisions VPC networks, subnets, firewall rules, Cloud NAT/Router, and GPU Compute Engine VM instances. |
| **Cloud Identity-Aware Proxy (IAP) API** | `iap.googleapis.com` | Manages secure TCP port 22 SSH tunneling to the private VM without exposing public IP addresses. |
| **Artifact Registry API** | `artifactregistry.googleapis.com` | Stores and manages the HEIR Docker container images in Google Cloud. |
| **Cloud Build API** | `cloudbuild.googleapis.com` | Builds and pushes the Docker container image directly to Artifact Registry during `terraform apply`. |
| **Cloud IAM API** | `iam.googleapis.com` | Creates dedicated service accounts and configures fine-grained IAM policy bindings. |
| **Cloud Logging API** | `logging.googleapis.com` | Collects startup script execution logs, system logs, and Cloud NAT logs. |
| **Cloud Monitoring API** | `monitoring.googleapis.com` | Ingests VM health metrics, compute resource utilization, and GPU telemetry. |
| **Service Usage API** | `serviceusage.googleapis.com` | Enables and manages service APIs and project quota inspection. |

**Command to enable all required APIs:**
```bash
gcloud services enable \
  compute.googleapis.com \
  iap.googleapis.com \
  artifactregistry.googleapis.com \
  cloudbuild.googleapis.com \
  iam.googleapis.com \
  logging.googleapis.com \
  monitoring.googleapis.com \
  serviceusage.googleapis.com \
  --project="<YOUR_PROJECT_ID>"
```

### Required IAM Roles & Permissions

The deploying identity (user or CI/CD deployment service account) requires the following IAM roles to provision infrastructure, build container images, and manage access policies:

| Role Name | Role Identifier | Purpose / Description |
|---|---|---|
| **Compute Admin** | `roles/compute.admin` | Full control over VPCs, subnets, firewall rules, Cloud Router/NAT, and GPU VM instances. |
| **Artifact Registry Administrator** | `roles/artifactregistry.admin` | Full control over Artifact Registry repositories and container image management. |
| **Cloud Build Editor** | `roles/cloudbuild.builds.editor` | Permission to submit and run container builds via `gcloud builds submit`. |
| **Service Account Admin** | `roles/iam.serviceAccountAdmin` | Permission to create and manage the dedicated VM service account (`heir-gpu-vm-sa`). |
| **Service Account User** | `roles/iam.serviceAccountUser` | Permission to attach the VM service account to the compute instance. |
| **Project IAM Admin** | `roles/resourcemanager.projectIamAdmin` | Permission to assign project IAM roles to the VM service account and developer identity. |
| **Service Usage Consumer** | `roles/serviceusage.serviceUsageConsumer` | Permission to consume project APIs and validate service quotas. |

Additionally, developers/users connecting to the VM via SSH need:
| Role Name | Role Identifier | Purpose / Description |
|---|---|---|
| **IAP-secured Tunnel User** | `roles/iap.tunnelResourceAccessor` | Connects through Google IAP TCP tunneling (also provisioned in `main.tf` for `var.user_email`). |
| **Compute Viewer** | `roles/compute.viewer` | Reads instance metadata and discovers VM network endpoints for `gcloud compute ssh`. |

**Command to grant all required IAM roles:**
```bash
PROJECT_ID="<YOUR_PROJECT_ID>"
USER_EMAIL="<YOUR_USER_EMAIL>" # e.g. developer@example.com

for ROLE in \
  roles/compute.admin \
  roles/artifactregistry.admin \
  roles/cloudbuild.builds.editor \
  roles/iam.serviceAccountAdmin \
  roles/iam.serviceAccountUser \
  roles/resourcemanager.projectIamAdmin \
  roles/serviceusage.serviceUsageConsumer \
  roles/iap.tunnelResourceAccessor \
  roles/compute.viewer; do
  gcloud projects add-iam-policy-binding "$PROJECT_ID" \
    --member="user:$USER_EMAIL" \
    --role="$ROLE"
done
```

---

### Step 1: Align Your Settings
Open `terraform.tfvars.example`, create your `terraform.tfvars` file, and edit the core parameters:
```bash
cp terraform.tfvars.example terraform.tfvars
```

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
gcloud compute ssh {machine name here} --zone {your zone here} --tunnel-through-iap
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
sudo docker exec -it fhe-machine bazel --help
```

You can run mlir files if you have it in your directory
```bash
sudo docker exec -it fhe-machine bazel run //tools:heir-opt -- \
  --mlir-to-cggi add_one_lut3.mlir \
  -o add_one_cggi.mlir
```

### 5. Execute an example
Login into the docker instance

```bash
sudo docker exec -it fhe-machine /bin/bash
```

Use Bazel to execute it
```bash
bazel run //tools:heir-opt -- \
  --mlir-to-cggi $(pwd)/tests/Examples/jaxite/add_one_lut3.mlir \
  -o $(pwd)/add_one_cggi.mlir
```
