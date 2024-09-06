// example code from https://github.com/Azure/terraform/tree/master/quickstart/101-vm-with-infrastructure

provider "google" {
  project = "master-420616"
  region  = "europe-west3"
  zone    = "europe-west3-c"
}