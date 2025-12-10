#![no_main]
#![no_std]

extern crate alloc;

use alloy_primitives::U256;
use alloy_sol_types::SolValue;
//use alloc::vec::Vec;
use alloc::format;
////use alloc::string::String;
use risc0_zkvm::guest::env;
////use serde::{Deserialize, Serialize};
////use bincode;
////use x509_parser::prelude::*;
////use x509_parser::certificate::X509Certificate;
////use sha2::{Sha384, Digest};
////use rsa::pss::{VerifyingKey as RsaVerifyingKey};
////use rsa::pkcs1::DecodeRsaPublicKey;
////use rsa::signature::Verifier as RsaVerifier;
////use rsa::signature::DigestVerifier;
////use core::convert::TryFrom;
////use serde_json;
//use tdx_quote::Quote;
//use base64::Engine;
////use alloc::string::ToString;

risc0_zkvm::guest::entry!(main);

fn main() {
    let start = env::cycle_count();

    //let quote_bytes: Vec<u8> = env::read();
    //env::log(&format!("Received quote_bytes with length: {}", quote_bytes.len()));
    //
    //
    //// Verify the quote using the tdx_quote library.
    //let is_valid = match Quote::from_bytes(&quote_bytes) {
    //    Ok(quote) => {
    //        env::log("TDX quote parsed successfully.");
    //        // Log all fields of the TDX QuoteBody
    //        let body = &quote.body;
    //        fn to_hex(bytes: &[u8]) -> alloc::string::String {
    //            bytes.iter().map(|b| format!("{:02x}", b)).collect::<Vec<_>>().join("")
    //        }
    //        env::log(&format!("TDX Body: tdx_version: {:?}", body.tdx_version));
    //        env::log(&format!("tee_tcb_svn: {}", to_hex(&body.tee_tcb_svn)));
    //        env::log(&format!("mrseam: {}", to_hex(&body.mrseam)));
    //        env::log(&format!("mrsignerseam: {}", to_hex(&body.mrsignerseam)));
    //        env::log(&format!("seamattributes: {}", to_hex(&body.seamattributes)));
    //        env::log(&format!("tdattributes: {}", to_hex(&body.tdattributes)));
    //        env::log(&format!("xfam: {}", to_hex(&body.xfam)));
    //        env::log(&format!("mrtd: {}", to_hex(&body.mrtd)));
    //        env::log(&format!("mrconfigid: {}", to_hex(&body.mrconfigid)));
    //        env::log(&format!("mrowner: {}", to_hex(&body.mrowner)));
    //        env::log(&format!("mrownerconfig: {}", to_hex(&body.mrownerconfig)));
    //        env::log(&format!("rtmr0: {}", to_hex(&body.rtmr0)));
    //        env::log(&format!("rtmr1: {}", to_hex(&body.rtmr1)));
    //        env::log(&format!("rtmr2: {}", to_hex(&body.rtmr2)));
    //        env::log(&format!("rtmr3: {}", to_hex(&body.rtmr3)));
    //        env::log(&format!("reportdata: {}", to_hex(&body.reportdata)));
    //        if let Some(tee_tcb_svn_2) = &body.tee_tcb_svn_2 {
    //            env::log(&format!("tee_tcb_svn_2: {}", to_hex(tee_tcb_svn_2)));
    //        }
    //        if let Some(mrservicetd) = &body.mrservicetd {
    //            env::log(&format!("mrservicetd: {}", to_hex(mrservicetd)));
    //        }
    //        if let Ok(cert_chain) = quote.pck_cert_chain() {
    //            // Try PEM first, then DER-to-PEM fallback
    //            let as_str = core::str::from_utf8(&cert_chain);
    //            if let Ok(s) = as_str {
    //                if s.contains("-----BEGIN CERTIFICATE-----") {
    //                    env::log(s);
    //                } else {
    //                    // Fallback: DER to PEM
    //                    fn der_to_pem_chain(der: &[u8]) -> alloc::string::String {
    //                        let mut pem = alloc::string::String::new();
    //                        let mut i = 0;
    //                        while i + 4 < der.len() {
    //                            if der[i] == 0x30 && der[i+1] == 0x82 {
    //                                let len = ((der[i+2] as usize) << 8) | (der[i+3] as usize);
    //                                let end = i + 4 + len;
    //                                if end <= der.len() {
    //                                    let cert = &der[i..end];
    //                                    let b64 = base64::engine::general_purpose::STANDARD.encode(cert);
    //                                    if !b64.is_empty() {
    //                                        pem.push_str("-----BEGIN CERTIFICATE-----\n");
    //                                        for chunk in b64.as_bytes().chunks(64) {
    //                                            pem.push_str(core::str::from_utf8(chunk).unwrap_or(""));
    //                                            pem.push('\n');
    //                                        }
    //                                        pem.push_str("-----END CERTIFICATE-----\n");
    //                                    }
    //                                    i = end;
    //                                    continue;
    //                                }
    //                            }
    //                            i += 1;
    //                        }
    //                        pem
    //                    }
    //                    let pem = der_to_pem_chain(&cert_chain);
    //                    if !pem.is_empty() {
    //                        env::log(&pem);
    //                    } else {
    //                        env::log(&format!("pck_cert_chain (hex): {}", to_hex(&cert_chain)));
    //                    }
    //                }
    //            } else {
    //                // Not valid UTF-8, fallback to DER-to-PEM
    //                fn der_to_pem_chain(der: &[u8]) -> alloc::string::String {
    //                    let mut pem = alloc::string::String::new();
    //                    let mut i = 0;
    //                    while i + 4 < der.len() {
    //                        if der[i] == 0x30 && der[i+1] == 0x82 {
    //                            let len = ((der[i+2] as usize) << 8) | (der[i+3] as usize);
    //                            let end = i + 4 + len;
    //                            if end <= der.len() {
    //                                let cert = &der[i..end];
    //                                let b64 = base64::engine::general_purpose::STANDARD.encode(cert);
    //                                if !b64.is_empty() {
    //                                    pem.push_str("-----BEGIN CERTIFICATE-----\n");
    //                                    for chunk in b64.as_bytes().chunks(64) {
    //                                        pem.push_str(core::str::from_utf8(chunk).unwrap_or(""));
    //                                        pem.push('\n');
    //                                    }
    //                                    pem.push_str("-----END CERTIFICATE-----\n");
    //                                }
    //                                i = end;
    //                                continue;
    //                            }
    //                        }
    //                        i += 1;
    //                    }
    //                    pem
    //                }
    //                let pem = der_to_pem_chain(&cert_chain);
    //                if !pem.is_empty() {
    //                    env::log(&pem);
    //                } else {
    //                    env::log(&format!("pck_cert_chain (hex): {}", to_hex(&cert_chain)));
    //                }
    //            }
    //        } else {
    //            env::log("No PCK certificate chain found in quote.");
    //        }
    //        // The quote is verified using the embedded PCK certificate chain.
    //        match quote.verify() {
    //            Ok(_pck) => {
    //                env::log("TDX attestation verification successful.");
    //                true
    //            }
    //            Err(e) => {
    //                env::log(&format!("TDX attestation verification failed: {:?}", e));
    //                false
    //            }
    //        }
    //    }
    //    Err(e) => {
    //        env::log(&format!("TDX quote parsing failed: {:?}", e));
    //        false
    //    }
    //};

    //// Write the result to the journal
    //let output: u32 = if is_valid { 1 } else { 0 };
    //let output: U256 = if is_valid { U256::from(1u32) } else { U256::from(0u32) };
    //let output: u32 = 1;
    let output: U256 = U256::from(1u32);
    env::log(format!("Output: {:?}", output).as_str());

    let end = env::cycle_count();
    env::log(format!("Cycle count: {}", end - start).as_str());
   
    //env::commit(&output);
    env::commit_slice(output.abi_encode().as_slice());
}
