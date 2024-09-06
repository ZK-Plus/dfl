// Output the external IP addresses of the Compute Engine instances
output "public_ip_addresses" {
  value = { for instance in google_compute_instance.my_terraform_vm : instance.name => instance.network_interface[0].access_config[0].nat_ip }
  description = "The public IP addresses of the VMs"
}