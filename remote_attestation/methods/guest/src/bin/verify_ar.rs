#![no_main]
#![no_std]

extern crate alloc;

use alloy_sol_types::SolValue;
use alloc::vec::Vec;
use alloc::format;
//use alloc::string::String;
use risc0_zkvm::guest::env;
//use serde::{Deserialize, Serialize};
//use bincode;
//use x509_parser::prelude::*;
//use x509_parser::certificate::X509Certificate;
//use sha2::{Sha384, Digest};
//use rsa::pss::{VerifyingKey as RsaVerifyingKey};
//use rsa::pkcs1::DecodeRsaPublicKey;
//use rsa::signature::Verifier as RsaVerifier;
//use rsa::signature::DigestVerifier;
//use core::convert::TryFrom;
//use serde_json;
use tdx_quote::Quote;
use base64::Engine;
//use alloc::string::ToString;

risc0_zkvm::guest::entry!(main);

 /* If the body is needed
#[derive(Serialize, Deserialize, Debug)]
struct TDXBody {
    tee_tcb_svn: String,
    mr_seam: String,
    mr_signer_seam: String,
    seam_attributes: String,
    td_attributes: String,
    xfam: String,
    mr_td: String,
    mr_config_id: String,
    mr_owner: String,
    mr_owner_config: String,
    rt_mr0: String,
    rt_mr1: String,
    rt_mr2: String,
    rt_mr3: String,
    report_data: String,
}*/

/* This is for ARMs Remote Attestation.
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
}*/

fn main() {
    let start = env::cycle_count();
    // Read the serialized input data
    // let mut serialized_data = Vec::<u8>::new();
    // env::stdin().read_to_end(&mut serialized_data).unwrap();
    //let serialized_data: Vec<u8> = env::read();//rtxbody
    let quote_bytes: Vec<u8> = env::read();
    env::log(&format!("Received quote_bytes with length: {}", quote_bytes.len()));

    /* If the body is needed
    // Deserialize the input data from JSON
    let report: TDXBody = match serde_json::from_slice(&serialized_data) {
        Ok(data) => data,
        Err(err) => {
            env::log(format!("Deserialization from JSON failed: {:?}", err).as_str());
            env::commit_slice(&[0u8; 32]); // Verification failed
            return;
        }
    };
    env::log(format!("TD Report Body deserialized successfully: {:?}", report).as_str());*/

    // For now, we'll just commit a success value.
    //let is_valid = true;

    // Verify the quote using the tdx_quote library.
    let is_valid = match Quote::from_bytes(&quote_bytes) {
        Ok(quote) => {
            env::log("TDX quote parsed successfully.");
            // Log all fields of the TDX QuoteBody
            let body = &quote.body;
            fn to_hex(bytes: &[u8]) -> alloc::string::String {
                bytes.iter().map(|b| format!("{:02x}", b)).collect::<Vec<_>>().join("")
            }
            env::log(&format!("TDX Body: tdx_version: {:?}", body.tdx_version));
            env::log(&format!("tee_tcb_svn: {}", to_hex(&body.tee_tcb_svn)));
            env::log(&format!("mrseam: {}", to_hex(&body.mrseam)));
            env::log(&format!("mrsignerseam: {}", to_hex(&body.mrsignerseam)));
            env::log(&format!("seamattributes: {}", to_hex(&body.seamattributes)));
            env::log(&format!("tdattributes: {}", to_hex(&body.tdattributes)));
            env::log(&format!("xfam: {}", to_hex(&body.xfam)));
            env::log(&format!("mrtd: {}", to_hex(&body.mrtd)));
            env::log(&format!("mrconfigid: {}", to_hex(&body.mrconfigid)));
            env::log(&format!("mrowner: {}", to_hex(&body.mrowner)));
            env::log(&format!("mrownerconfig: {}", to_hex(&body.mrownerconfig)));
            env::log(&format!("rtmr0: {}", to_hex(&body.rtmr0)));
            env::log(&format!("rtmr1: {}", to_hex(&body.rtmr1)));
            env::log(&format!("rtmr2: {}", to_hex(&body.rtmr2)));
            env::log(&format!("rtmr3: {}", to_hex(&body.rtmr3)));
            env::log(&format!("reportdata: {}", to_hex(&body.reportdata)));
            if let Some(tee_tcb_svn_2) = &body.tee_tcb_svn_2 {
                env::log(&format!("tee_tcb_svn_2: {}", to_hex(tee_tcb_svn_2)));
            }
            if let Some(mrservicetd) = &body.mrservicetd {
                env::log(&format!("mrservicetd: {}", to_hex(mrservicetd)));
            }
            if let Ok(cert_chain) = quote.pck_cert_chain() {
                // Try PEM first, then DER-to-PEM fallback
                let as_str = core::str::from_utf8(&cert_chain);
                if let Ok(s) = as_str {
                    if s.contains("-----BEGIN CERTIFICATE-----") {
                        env::log(s);
                    } else {
                        // Fallback: DER to PEM
                        fn der_to_pem_chain(der: &[u8]) -> alloc::string::String {
                            let mut pem = alloc::string::String::new();
                            let mut i = 0;
                            while i + 4 < der.len() {
                                if der[i] == 0x30 && der[i+1] == 0x82 {
                                    let len = ((der[i+2] as usize) << 8) | (der[i+3] as usize);
                                    let end = i + 4 + len;
                                    if end <= der.len() {
                                        let cert = &der[i..end];
                                        let b64 = base64::engine::general_purpose::STANDARD.encode(cert);
                                        if !b64.is_empty() {
                                            pem.push_str("-----BEGIN CERTIFICATE-----\n");
                                            for chunk in b64.as_bytes().chunks(64) {
                                                pem.push_str(core::str::from_utf8(chunk).unwrap_or(""));
                                                pem.push('\n');
                                            }
                                            pem.push_str("-----END CERTIFICATE-----\n");
                                        }
                                        i = end;
                                        continue;
                                    }
                                }
                                i += 1;
                            }
                            pem
                        }
                        let pem = der_to_pem_chain(&cert_chain);
                        if !pem.is_empty() {
                            env::log(&pem);
                        } else {
                            env::log(&format!("pck_cert_chain (hex): {}", to_hex(&cert_chain)));
                        }
                    }
                } else {
                    // Not valid UTF-8, fallback to DER-to-PEM
                    fn der_to_pem_chain(der: &[u8]) -> alloc::string::String {
                        let mut pem = alloc::string::String::new();
                        let mut i = 0;
                        while i + 4 < der.len() {
                            if der[i] == 0x30 && der[i+1] == 0x82 {
                                let len = ((der[i+2] as usize) << 8) | (der[i+3] as usize);
                                let end = i + 4 + len;
                                if end <= der.len() {
                                    let cert = &der[i..end];
                                    let b64 = base64::engine::general_purpose::STANDARD.encode(cert);
                                    if !b64.is_empty() {
                                        pem.push_str("-----BEGIN CERTIFICATE-----\n");
                                        for chunk in b64.as_bytes().chunks(64) {
                                            pem.push_str(core::str::from_utf8(chunk).unwrap_or(""));
                                            pem.push('\n');
                                        }
                                        pem.push_str("-----END CERTIFICATE-----\n");
                                    }
                                    i = end;
                                    continue;
                                }
                            }
                            i += 1;
                        }
                        pem
                    }
                    let pem = der_to_pem_chain(&cert_chain);
                    if !pem.is_empty() {
                        env::log(&pem);
                    } else {
                        env::log(&format!("pck_cert_chain (hex): {}", to_hex(&cert_chain)));
                    }
                }
            } else {
                env::log("No PCK certificate chain found in quote.");
            }
            // The quote is verified using the embedded PCK certificate chain.
            match quote.verify() {
                Ok(_pck) => {
                    env::log("TDX attestation verification successful.");
                    true
                }
                Err(e) => {
                    env::log(&format!("TDX attestation verification failed: {:?}", e));
                    false
                }
            }
        }
        Err(e) => {
            env::log(&format!("TDX quote parsing failed: {:?}", e));
            false
        }
    };

    /* This is for ARMs Remote Attestation.
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
    }*/


    // Write the result to the journal
    let output: u32 = if is_valid { 1 } else { 0 };
    env::log(format!("Output: {:?}", output).as_str());

    let end = env::cycle_count();
    env::log(format!("Cycle count: {}", end - start).as_str());
   
    //env::commit(&output);
    env::commit_slice(output.abi_encode().as_slice());
}
