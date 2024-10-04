# example code from https://github.com/Azure/terraform/tree/master/quickstart/101-vm-with-infrastructure
resource "random_pet" "rg_name" {
  prefix = var.resource_group_name_prefix
}

# create vpc network
resource "google_compute_network" "vpc_network" {
  project                 = var.project_id
  name                    = "fl-network"
  auto_create_subnetworks = false
  mtu                     = 1460
}

resource "google_compute_subnetwork" "default" {
  name          = "my-custom-subnet"
  project       = var.project_id
  ip_cidr_range = "10.0.1.0/24"
  region        = var.location_region
  network       = google_compute_network.vpc_network.id
}

# Create public IPs for each client VM
resource "google_compute_address" "my_terraform_public_ip" {
  count  = var.vm_count
  name   = count.index == 0 ? "aggregator" : format("client%d", count.index)
  project = var.project_id
  region = var.location_region
}


# Create Network Security Group and rule
resource "google_compute_firewall" "my_terraform_firewall" {
  name    = "fl-network-firewall"
  project = var.project_id
  network = google_compute_network.vpc_network.self_link

  allow {
    protocol = "tcp"
    ports    = ["22", "80", "5555"]
  }

  source_ranges = ["0.0.0.0/0"]
}

# create VM instances
resource "google_compute_instance" "my_terraform_vm" {
  provider     = google-beta
  count        = var.vm_count
  name         = count.index == 0 ? "aggregator" : format("client%d", count.index)
  machine_type = "n2d-standard-2"
  min_cpu_platform = "AMD Milan"
  zone         = var.location_zone

  can_ip_forward      = false
  deletion_protection = false
  enable_display      = false

  # Conditionally include the confidential_instance_config block
  dynamic "confidential_instance_config" {
    for_each = var.enable_sev_snp_confidentiality ? [1] : []
    content {
      enable_confidential_compute = true
      confidential_instance_type  = "SEV_SNP"
    }
  }

  #Disable vTPM and Integrity Monitoring for performance improvement
  shielded_instance_config {
    enable_secure_boot          = false
    enable_vtpm                 = false
    enable_integrity_monitoring = false
  }


  boot_disk {
    auto_delete = true
    initialize_params {
      image = "projects/ubuntu-os-cloud/global/images/ubuntu-2004-focal-v20240426"
      size  = 10
      type  = "pd-standard"
    }
  }

  network_interface {
    network = google_compute_network.vpc_network.id
    subnetwork = google_compute_subnetwork.default.id
    access_config {
      // Leaving this block empty will assign a ephemeral public IP
    }
  }

  metadata = {
    ssh-keys = "${var.username}:${trimspace(file(var.public_key_path))}"
    enable-oslogin = "FALSE"
  }

  scheduling {
    preemptible = true
    automatic_restart = false
    on_host_maintenance = "TERMINATE"
  }
}
