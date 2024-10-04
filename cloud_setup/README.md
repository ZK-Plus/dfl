# Cloud Setup

This repository contains the necessary configurations and scripts to set up and manage cloud infrastructure using Terraform and Ansible.

## Folder Structure

- `terraform/`

  - Contains Terraform scripts for provisioning cloud resources.
  - Ensure you have the necessary provider configurations and credentials before running the scripts.
  - By configuring the variables you can specify further VM properties.

- `ansible/`
  - Contains Ansible playbooks for configuring and managing the provisioned cloud resources..
  - Make sure to update the inventory file with the correct host information.

## Getting Started

1. **Terraform Setup**

   - Navigate to the `terraform/` directory.
   - Initialize Terraform: `terraform init`
   - Plan the infrastructure changes: `terraform plan`
   - Apply the changes: `terraform apply`

2. **Ansible Setup**
   - Navigate to the `ansible/` directory.
   - Update the inventory file with the target hosts.
   - Run the playbooks: `ansible-playbook -i hosts.ini prepare.yml -vv`

## Prerequisites

- [Terraform](https://www.terraform.io/downloads.html) installed
- [Ansible](https://docs.ansible.com/ansible/latest/installation_guide/intro_installation.html) installed
- Proper cloud provider credentials configured

## License

This project is licensed under the MIT License.
