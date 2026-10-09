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

output "instance_name" {
  value       = google_compute_instance.gpu_vm.name
  description = "The name of the deployed GCE confidential GPU instance."
}

output "instance_self_link" {
  value       = google_compute_instance.gpu_vm.self_link
  description = "The self-link of the deployed GCE confidential GPU instance."
}

output "instance_zone" {
  value       = google_compute_instance.gpu_vm.zone
  description = "The zone where the instance is running."
}

output "instance_private_ip" {
  value       = google_compute_instance.gpu_vm.network_interface[0].network_ip
  description = "The internal (private) IP address of the deployed instance."
}

output "vpc_id" {
  value       = module.vpc.vpc_id
  description = "The ID of the custom VPC network."
}

output "subnet_id" {
  value       = module.vpc.subnet_id
  description = "The ID of the custom subnet."
}
