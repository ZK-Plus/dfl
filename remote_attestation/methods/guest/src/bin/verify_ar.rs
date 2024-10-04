#![no_main]
#![no_std]

extern crate alloc;

use alloy_sol_types::SolValue;
use alloc::vec::Vec;
use alloc::format;
use risc0_zkvm::guest::env;
use serde::{Deserialize, Serialize};
use bincode;
use x509_parser::prelude::*;
use x509_parser::certificate::X509Certificate;
use sha2::{Sha384, Digest};
use rsa::pss::{VerifyingKey as RsaVerifyingKey};
use rsa::pkcs1::DecodeRsaPublicKey;
use rsa::signature::Verifier as RsaVerifier;
use rsa::signature::DigestVerifier;
use core::convert::TryFrom;

risc0_zkvm::guest::entry!(main);

#[derive(Serialize, Deserialize)]
struct InputData {
    vcek: Vec<u8>,
    cert_chain: Vec<u8>,
    public_key: Vec<u8>,
}

fn verify_rsa_signature(
    public_key: rsa::RsaPublicKey,
    tbs_cert: &[u8],
    signature: &[u8],
    salt_len: usize,
) -> bool {

    let verifying_key = RsaVerifyingKey::<Sha384>::new_with_salt_len(public_key, salt_len);
    env::log(format!("Verifying key: {:?}", verifying_key).as_str());

    let signature_value = match rsa::pss::Signature::try_from(signature) {
        Ok(signature) => signature,
        Err(err) => {
            env::log(format!("Failed to decode signature: {:?}", err).as_str());
            return false;
        }
    };

    // Create a new digest instance with the provided digest
    //let digest_instance = Sha384::new_with_prefix(digest);

    // Verify the digest using the verifying key and the signature value
    let result = verifying_key.verify(tbs_cert, &signature_value);

    result.is_ok()
}

fn main() {
    let start = env::cycle_count();
    // Read the serialized input data
    // let mut serialized_data = Vec::<u8>::new();
    // env::stdin().read_to_end(&mut serialized_data).unwrap();
    let serialized_data: Vec<u8> = env::read();

    // Deserialize the input data
    let input_data: InputData = match bincode::deserialize(&serialized_data) {
        Ok(data) => data,
        Err(err) => {
            env::log(format!("Deserialization failed: {:?}", err).as_str());
            env::commit(&0); // Verification failed
            return;
        }
    };

    // Parse the CA certificate
    let (_, ca_cert) = match X509Certificate::from_der(&input_data.cert_chain) {
        Ok(cert) => cert,
        Err(err) => {
            env::log(format!("Failed to parse CA cert: {:?}", err).as_str());
            env::commit(&0); // Verification failed
            return;
        }
    };
    env::log("CA certificate parsed successfully.");

    // Parse the VCEK certificate
    let (_, vcek_cert) = match X509Certificate::from_der(&input_data.vcek) {
        Ok(cert) => cert,
        Err(err) => {
            env::log(format!("Failed to parse VCEK cert: {:?}", err).as_str());
            env::commit(&0); // Verification failed
            return;
        }
    };
    env::log("VCEK certificate parsed successfully.");


    // Extract the SubjectPublicKeyInfo from the CA certificate
    let ca_public_key_info = ca_cert.tbs_certificate.subject_pki;
    env::log("Extracted SubjectPublicKeyInfo from CA certificate.");

    // Create the RsaPublicKey from the CA certificate's public key
    let ca_public_key_bytes = ca_public_key_info.subject_public_key.data;
    env::log(format!("CA public key bytes: {:?}", ca_public_key_bytes).as_str());
    let ca_public_key = match rsa::RsaPublicKey::from_pkcs1_der(ca_public_key_bytes) {
        Ok(pub_key) => pub_key,
        Err(err) => {
            env::log(format!("Failed to decode RsaPublicKey from CA: {:?}", err).as_str());
            env::commit(&0); // Verification failed
            return;
        }
    };
    env::log("Decoded RsaPublicKey from CA certificate.");

    // Create a SHA-384 digest of the VCEK certificate's TBS (To Be Signed) data
    let tbs_cert = vcek_cert.tbs_certificate.as_ref();
    let mut hasher = Sha384::new();
    hasher.update(tbs_cert);
    let digest = hasher.finalize();
    env::log(format!("Digest: {:?}", digest).as_str());

    // Verify the signature using RSASSA-PSS with SHA-384
    let salt_len = 48; // Salt length as specified in the VCEK
    let signature_bytes = vcek_cert.signature_value.data;
    env::log(format!("Signature bytes: {:?}", signature_bytes).as_str());
    let is_valid = verify_rsa_signature(ca_public_key, &tbs_cert, &signature_bytes, salt_len);

    if is_valid {
        env::log("Verification of VCEK successful.");
    } else {
        env::log("Verification of VCEK failed.");
    }


    // Write the result to the journal
    let output: u32 = if is_valid { 1 } else { 0 };
    env::log(format!("Output: {:?}", output).as_str());

    let end = env::cycle_count();
    env::log(format!("Cycle count: {}", end - start).as_str());
   
    //env::commit(&output);
    env::commit_slice(output.abi_encode().as_slice());
}
