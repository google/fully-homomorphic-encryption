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
locals {
  # 1. Detect if running on Windows by checking if the absolute path starts with a drive letter (e.g., C:/)
  is_windows = length(regexall("^[a-zA-Z]:", abspath(path.module))) > 0

  # 2. Define the correct shell interpreter per OS
  shell_interpreter = local.is_windows ? ["PowerShell", "-Command"] : ["bash", "-l", "-c"]

  # 3. Define the correct command per OS
  # Notice: 'heir' is replaced with ${google_artifact_registry_repository.heir_registry.repository_id}
  # 1. Detect if running on Windows by checking if the absolute path starts with a drive letter (e.g., C:/)
  cmd_linux   = "PATH=$PATH:/usr/local/bin:/opt/homebrew/bin:$HOME/google-cloud-sdk/bin gcloud builds submit --tag ${var.artifact_registry_region}-docker.pkg.dev/${var.project_id}/${var.image_name} ${path.module}"
  cmd_windows = "gcloud.cmd builds submit --tag ${var.artifact_registry_region}-docker.pkg.dev/${var.project_id}/${var.image_name} ${path.module}"
  build_command = local.is_windows ? local.cmd_windows : local.cmd_linux
}
module "vpc" {
  source     = "./modules/vpc"
  project_id = var.project_id
  region     = var.region
}

resource "google_artifact_registry_repository" "heir_registry" {
  location      = var.artifact_registry_region
  repository_id = "heir"
  description   = "Docker registry for HEIR compiler"
  format        = "DOCKER"
}

resource "null_resource" "build_and_push_docker" {
  # Ensures the registry exists before trying to push to it
  depends_on = [
    google_artifact_registry_repository.heir_registry
  ]

  # Only triggers a new build when the Dockerfile or parameters change
  triggers = {
    dockerfile_hash = filemd5("${path.module}/Dockerfile")
    project_id      = var.project_id
    image_name      = var.image_name
    registry_region = var.artifact_registry_region
    repository_id   = google_artifact_registry_repository.heir_registry.repository_id
  }

  provisioner "local-exec" {
    # Dynamically applies the correct OS shell and command
    interpreter = local.shell_interpreter
    command     = local.build_command
  }
}

resource "google_service_account" "gpu_vm_sa" {
  account_id   = "heir-gpu-vm-sa"
  display_name = "Dedicated Service Account for HEIR GPU VM"
  project      = var.project_id
}

resource "google_project_iam_member" "gpu_vm_sa_roles" {
  for_each = toset([
    "roles/logging.logWriter",
    "roles/monitoring.metricWriter",
    "roles/artifactregistry.reader"
  ])
  project = var.project_id
  role    = each.value
  member  = "serviceAccount:${google_service_account.gpu_vm_sa.email}"
}

resource "google_compute_instance" "gpu_vm" {
  name         = var.instance_name
  machine_type = var.machine_type
  zone         = var.zone
  project      = var.project_id

  boot_disk {
    initialize_params {
      image = var.boot_image
      size  = var.boot_disk_size
      type  = var.boot_disk_type
    }
  }

  guest_accelerator {
    type  = var.gpu_type
    count = var.gpu_count
  }

  network_interface {
    network    = module.vpc.vpc_id
    subnetwork = module.vpc.subnet_id
  }

  scheduling {
    preemptible         = true
    provisioning_model  = "SPOT"
    automatic_restart   = false
    on_host_maintenance = "TERMINATE"
  }

  service_account {
    email  = google_service_account.gpu_vm_sa.email
    scopes = ["cloud-platform"]
  }

  dynamic "confidential_instance_config" {
    for_each = var.enable_confidential_vm ? [1] : []
    content {
      enable_confidential_compute = true
      confidential_instance_type  = "TDX"
    }
  }
  depends_on = [
    null_resource.build_and_push_docker
  ]

  metadata_startup_script = <<-EOT
      #!/bin/bash
      # Redirect all output to a log file for auditing and troubleshooting
      exec > /var/log/gpu-startup.log 2>&1
      set -e

      # If setup is already finished, exit immediately on subsequent boots
      if [ -f "/etc/startup_complete" ]; then
          echo "Startup already complete. Exiting."
          exit 0
      fi

      # PHASE 1: Install NVIDIA Drivers and Reboot
      if [ ! -f "/etc/gpu_installed" ]; then
          echo "Phase 1: Installing standard NVIDIA drivers..."
          apt-get update
          apt-get install -y ubuntu-drivers-common
          ubuntu-drivers autoinstall

          # Mark phase 1 as complete and reboot
          touch /etc/gpu_installed
          echo "Rebooting to load newly built GPU kernel modules..."
          reboot
          exit 0
      fi

      # PHASE 2: Install Docker, NVIDIA Container Toolkit, and run HEIR image
      echo "Phase 2: Installing Docker Engine..."
      curl -fsSL https://get.docker.com -o get-docker.sh
      sh get-docker.sh

      echo "Installing NVIDIA Container Toolkit..."
      curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg
      curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list | sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | tee /etc/apt/sources.list.d/nvidia-container-toolkit.list

      apt-get update
      apt-get install -y nvidia-container-toolkit
      nvidia-ctk runtime configure --runtime=docker
      systemctl restart docker

      echo "Configuring Docker credential helper for GCP..."
      gcloud auth configure-docker ${var.artifact_registry_region}-docker.pkg.dev -q

      echo "Pulling and executing the compiler container..."
      IMAGE_URL="${var.artifact_registry_region}-docker.pkg.dev/${var.project_id}/${var.image_name}"
      docker pull $IMAGE_URL

      # Run the container with all GPUs exposed
      docker run -d \
        --name fhe-machine \
        --gpus all \
        --entrypoint tail \
        --restart unless-stopped \
        --security-opt seccomp=unconfined \
        $IMAGE_URL \
        -f /dev/null

      # Mark the startup process as finished
      touch /etc/startup_complete
      echo "GPU setup and application launch complete!"
    EOT
}
