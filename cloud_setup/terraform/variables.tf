// example code from https://github.com/Azure/terraform/tree/master/quickstart/101-vm-with-infrastructure
variable "location_region" {
  type        = string
  default     = "europe-west3"
  description = "Location of the resource group."
  nullable = false
}

variable "location_zone" {
  type        = string
  default     = "europe-west3-c"
  description = "Zone of the resource group."
  nullable = false
}

variable "public_key_path" {
  type        = string
  description = "Path to the public key file that will be used to authenticate the SSH connection to the VM."
  default     = "/Users/johann/.ssh/gcloud.pub"
  nullable = false
}

variable "project_id" {
  type        = string
  description = "The project ID where the resources will be created."
  default     = "master-420616"
}

variable "resource_group_name_prefix" {
  type        = string
  default     = "rg"
  description = "Prefix of the resource group name that's combined with a random ID so name is unique in your Azure subscription."
  nullable = false
}

variable "username" {
  type        = string
  description = "The username for the local account that will be created on the new VM."
  default     = "johann"
  nullable = false
}

variable "vm_count" {
  type       = number
  description = "The number of virtual machines including the aggregator."
  default     = 1
  nullable = false
}

variable "enable_sev_snp_confidentiality" {
  description = "Enable AMD SEV-SNP Confidential Compute"
  type        = bool
  default     = false
}