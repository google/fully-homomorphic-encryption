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
  description = "The Google Cloud Project ID where the network resources will be created."
}

variable "region" {
  type        = string
  description = "The target GCP region for the subnet, Cloud Router, and Cloud NAT gateway."
}

variable "vpc_name" {
  type        = string
  default     = "iap-enabled-vpc"
  description = "The name of the custom VPC network."
}

variable "subnet_name" {
  type        = string
  default     = "iap-enabled-subnet"
  description = "The name of the subnetwork inside the VPC."
}

variable "ip_cidr_range" {
  type        = string
  default     = "10.0.1.0/24"
  description = "The primary IP CIDR range for the subnet."
}
