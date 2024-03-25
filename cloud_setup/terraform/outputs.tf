output "resource_group_name" {
  value = azurerm_resource_group.rg.name
}

output "public_ip_addresses" {
  value = { for ip in azurerm_linux_virtual_machine.my_terraform_vm : ip.name => ip.public_ip_address }
  description = "The public IP addresses of the VMs"
}

output "private_key_data" {
  value     = jsondecode(azapi_resource_action.ssh_public_key_gen.output).privateKey
  sensitive = true
}