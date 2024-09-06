# Dieser Code ist mit Terraform 4.25.0 und Versionen kompatibel, die mit 4.25.0 abwärtskompatibel sind.
# Informationen zum Validieren dieses Terraform-Codes finden Sie unter https://developer.hashicorp.com/terraform/tutorials/gcp-get-started/google-cloud-platform-build#format-and-validate-the-configuration.
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
  name   = "zkp-public-ip"
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



resource "google_compute_instance" "zkp" {
  boot_disk {
    auto_delete = true
    device_name = "zkp"

    initialize_params {
      image = "projects/debian-cloud/global/images/debian-12-bookworm-v20240709"
      size  = 10
      type  = "pd-balanced"
    }

    mode = "READ_WRITE"
  }

  can_ip_forward      = false
  deletion_protection = false
  enable_display      = false

  labels = {
    goog-ec-src = "vm_add-tf"
  }

  machine_type = "e2-standard-4"
  name         = "zkp"
  zone         = var.location_zone

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
    automatic_restart   = true
    on_host_maintenance = "MIGRATE"
    preemptible         = false
    provisioning_model  = "STANDARD"
  }

#   service_account {
#     email  = "292152220260-compute@developer.gserviceaccount.com"
#     scopes = ["https://www.googleapis.com/auth/devstorage.read_only", "https://www.googleapis.com/auth/logging.write", "https://www.googleapis.com/auth/monitoring.write", "https://www.googleapis.com/auth/service.management.readonly", "https://www.googleapis.com/auth/servicecontrol", "https://www.googleapis.com/auth/trace.append"]
#   }

  shielded_instance_config {
    enable_integrity_monitoring = true
    enable_secure_boot          = false
    enable_vtpm                 = true
  }

}
