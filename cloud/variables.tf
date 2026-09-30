# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

variable "project_id" {
  type        = string
  description = "The Google Cloud Project ID where all resources will be deployed."
}

variable "enable_confidential_vm" {
  description = "Set to true to enable AMD SEV/Intel TDX hardware encryption"
  type        = bool
  default     = false
}

variable "region" {
  type        = string
  default     = "us-west1"
  description = "The target GCP region for the custom VPC, subnetwork, router, and NAT."
}

variable "zone" {
  type        = string
  default     = "us-west1-b"
  description = "The target GCP zone where the confidential GPU instance will be deployed."
}

variable "instance_name" {
  type        = string
  default     = "l4-gpu-vm"
  description = "The name of the GCE confidential GPU instance."
}

variable "machine_type" {
  type        = string
  default     = "n1-standard-4"
  description = <<EOF
The machine type used for the GPU VM.
Must be compatible with the selected GPU accelerator.
- For Tesla T4 (nvidia-tesla-t4): 'n1-standard-4' (or larger N1 types) is recommended.
- For NVIDIA L4 (nvidia-l4): 'g2-standard-4' (or larger G2 types) is recommended.
- For NVIDIA H100 (nvidia-h100-80gb): 'a3-highgpu-1g' is recommended.
EOF
}

variable "gpu_type" {
  type        = string
  default     = "nvidia-tesla-t4"
  description = "The type of physical GPU accelerator to attach (e.g. 'nvidia-tesla-t4', 'nvidia-l4')."
}

variable "gpu_count" {
  type        = number
  default     = 1
  description = "The number of physical GPU accelerators to attach to the VM."
}

variable "boot_image" {
  type        = string
  default     = "ubuntu-os-cloud/ubuntu-2204-lts"
  description = "The boot image for the VM (Ubuntu 22.04 LTS is highly recommended for Docker/NVIDIA drivers)."
}

variable "boot_disk_size" {
  type        = number
  default     = 250
  description = "The boot disk size in gigabytes (GB)."
}

variable "boot_disk_type" {
  type        = string
  default     = "pd-balanced"
  description = "The boot disk type (e.g. 'pd-standard', 'pd-balanced', 'pd-ssd')."
  validation {
    condition     = contains(["pd-standard", "pd-balanced", "pd-ssd", "pd-extreme"], var.boot_disk_type)
    error_message = "The boot disk type must be one of: pd-standard, pd-balanced, pd-ssd, pd-extreme."
  }
}

variable "user_email" {
  type        = string
  default     = "leonardocifuentes@clsecteam.com"
  description = "The Google Account email of the user/developer who needs secure IAP SSH access to the VM."
}

variable "artifact_registry_region" {
  type        = string
  default     = "us-central1"
  description = "The GCP region containing the Artifact Registry repository."
}

variable "image_name" {
  type        = string
  default     = "heir/heirimage:latest"
  description = "The Artifact Registry container image path (e.g., 'heir/heirimage:latest')."
}
