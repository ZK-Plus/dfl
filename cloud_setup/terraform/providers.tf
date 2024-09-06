// example code from https://github.com/Azure/terraform/tree/master/quickstart/101-vm-with-infrastructure
terraform {
  required_version = ">=0.12"

  required_providers {
    google-beta = {
      source  = "hashicorp/google-beta"
      version = "~> 5.26" 
    }
  
    random = {
      source  = "hashicorp/random"
      version = "~>3.0"
    }
  }
}

provider "google-beta" {
  project = "master-420616"
  region  = "europe-west3"
  zone    = "europe-west3-c"
}