# example code from https://github.com/Azure/terraform/tree/master/quickstart/101-vm-with-infrastructure
resource "random_pet" "rg_name" {
  prefix = var.resource_group_name_prefix
}

# create new resource group with random pet name
resource "azurerm_resource_group" "rg" {
  location = var.resource_group_location
  name     = random_pet.rg_name.id
}

# Create virtual network
resource "azurerm_virtual_network" "my_terraform_network" {
  name                = "flVnet"
  address_space       = ["10.0.0.0/16"]
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
}

# Create subnet
resource "azurerm_subnet" "my_terraform_subnet" {
  name                 = "flSubnet"
  resource_group_name  = azurerm_resource_group.rg.name
  virtual_network_name = azurerm_virtual_network.my_terraform_network.name
  address_prefixes     = ["10.0.1.0/24"]
}

# Create public IPs for each client VM
resource "azurerm_public_ip" "my_terraform_public_ip" {
  count               = var.vm_count
  name                = count.index == 0 ? "flPublicIPAggregator" : format("flPublicIPClient%d", count.index)
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
  allocation_method   = "Dynamic"
}

# Create Network Security Group and rule
resource "azurerm_network_security_group" "my_terraform_nsg" {
  name                = "flNetworkSecurityGroup"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name

  security_rule {
    name                       = "SSH"
    priority                   = 1001
    direction                  = "Inbound"
    access                     = "Allow"
    protocol                   = "Tcp"
    source_port_range          = "*"
    destination_port_range     = "22"
    source_address_prefix      = "*"
    destination_address_prefix = "*"
  }
  security_rule {
    name                       = "HTTP"
    priority                   = 1002
    direction                  = "Inbound"
    access                     = "Allow"
    protocol                   = "Tcp"
    source_port_range          = "*"
    destination_port_range     = "80"
    source_address_prefix      = "*"
    destination_address_prefix = "*"
  }
  security_rule {
    name                       = "TCP"
    priority                   = 1003
    direction                  = "Inbound"
    access                     = "Allow"
    protocol                   = "Tcp"
    source_port_range          = "*"
    destination_port_range     = "5555"
    source_address_prefix      = "*"
    destination_address_prefix = "*"
  }

}

# Create network interface for each VM
resource "azurerm_network_interface" "my_terraform_nic" {
  count               = var.vm_count
  name                = count.index == 0 ? "myNICAggregator" : format("myNICClient%d", count.index)
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name

  ip_configuration {
    name                          = "my_nic_configuration"
    subnet_id                     = azurerm_subnet.my_terraform_subnet.id
    private_ip_address_allocation = "Dynamic"
    public_ip_address_id          = azurerm_public_ip.my_terraform_public_ip[count.index].id
  }
}

# Connect the security group to the network interface
resource "azurerm_network_interface_security_group_association" "example" {
  count                     = var.vm_count
  network_interface_id      = azurerm_network_interface.my_terraform_nic[count.index].id
  network_security_group_id = azurerm_network_security_group.my_terraform_nsg.id
}

# Generate random text for a unique storage account name
resource "random_id" "random_id" {
  keepers = {
    # Generate a new ID only when a new resource group is defined
    resource_group = azurerm_resource_group.rg.name
  }

  byte_length = 8
}

# Create storage account for boot diagnostics
resource "azurerm_storage_account" "my_storage_account" {
  name                     = "diag${random_id.random_id.hex}"
  location                 = azurerm_resource_group.rg.location
  resource_group_name      = azurerm_resource_group.rg.name
  account_tier             = "Standard"
  account_replication_type = "LRS"
}

# Create virtual machines (one aggregator and four clients)
resource "azurerm_linux_virtual_machine" "my_terraform_vm" {
  count                 = var.vm_count
  name                  = count.index == 0 ? "aggregator" : format("client%d", count.index)
  location              = azurerm_resource_group.rg.location
  resource_group_name   = azurerm_resource_group.rg.name
  network_interface_ids = [azurerm_network_interface.my_terraform_nic[count.index].id]
  size                  = "Standard_B1s"

  os_disk {
    name                 = count.index == 0 ? "myOsDiskAggregator" : format("myOsDiskClient%d", count.index)
    caching              = "ReadWrite"
    storage_account_type = "Premium_LRS"
  }

  source_image_reference {
    publisher = "canonical"
    offer     = "0001-com-ubuntu-confidential-vm-jammy"
    sku       = "22_04-lts-cvm"
    version   = "latest"
  }

  computer_name  = count.index == 0 ? "hostnameAggregator" : format("hostnameClient%d", count.index)
  admin_username = var.username

  admin_ssh_key {
    username   = var.username
    public_key = file("/Users/johann/.ssh/azure.pub")
  }

  boot_diagnostics {
    storage_account_uri = azurerm_storage_account.my_storage_account.primary_blob_endpoint
  }
}

# resource "local_file" "my-keys" {
#   #count = var.my-instance-count
#   content = jsondecode(azapi_resource_action.ssh_public_key_gen.output).privateKey
#   filename = "/Users/johann/.ssh/terra.pem"
#   file_permission = "0600"
# }

# resource "local_file" "public-ips" {
#   #count = var.my-instance-count
#   content = jsondecode(azapi_resource_action.ssh_public_key_gen.output).publicKey
#   filename = "./publicKey.pub"
#   file_permission = "0644"
# }