// example code from https://github.com/Azure/terraform/tree/master/quickstart/101-vm-with-infrastructure
variable "resource_group_location" {
  type        = string
  default     = "germanywestcentral"
  description = "Location of the resource group."
  nullable = false
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
  default     = "azureadmin"
  nullable = false
}

variable "vm_count" {
  type       = number
  description = "The number of virtual machines including the aggregator."
  default     = 1
  nullable = false
}