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

output "vpc_id" {
  value       = google_compute_network.vpc.id
  description = "The self-link of the custom VPC network."
}

output "vpc_name" {
  value       = google_compute_network.vpc.name
  description = "The name of the custom VPC network."
}

output "subnet_id" {
  value       = google_compute_subnetwork.subnet.id
  description = "The self-link of the custom subnetwork."
}

output "subnet_name" {
  value       = google_compute_subnetwork.subnet.name
  description = "The name of the custom subnetwork."
}

output "subnet_region" {
  value       = google_compute_subnetwork.subnet.region
  description = "The region where the subnet was deployed."
}
