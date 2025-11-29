// Copyright 2024 RISC Zero, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This application demonstrates how to send an off-chain proof request
// to the Bonsai proving service and publish the received proofs directly
// to your deployed app contract.

use alloy_primitives::{U256};
use anyhow::{Context, Result};
use clap::Parser;
use ethers::prelude::*;
use methods::VERIFY_AR_ELF;
use risc0_ethereum_contracts::groth16;
use risc0_zkvm::{default_prover, ExecutorEnv, ProverOpts, VerifierContext};
use hex;
use alloy_sol_types::{sol, SolCall, SolInterface};
use std::fs;

// `IEvenNumber` interface automatically generated via the alloy `sol!` macro.
sol! {
    interface IDeviceRegistry {
        function registerDevice(
            uint256 x,
            bytes calldata seal,
            address _address,
            string memory _public_ip,
            string memory _msg_broker_ip,
            bytes memory _public_key
        );
        function runProof(bytes calldata seal, uint256 x);
    }
}

/// Wrapper of a `SignerMiddleware` client to send transactions to the given
/// contract's `Address`.
pub struct TxSender {
    chain_id: u64,
    client: SignerMiddleware<Provider<Http>, Wallet<k256::ecdsa::SigningKey>>,
    contract: Address,
}

impl TxSender {
    /// Creates a new `TxSender`.
    pub fn new(chain_id: u64, rpc_url: &str, private_key: &str, contract: &str) -> Result<Self> {
        let provider = Provider::<Http>::try_from(rpc_url)?;
        let wallet: LocalWallet = private_key.parse::<LocalWallet>()?.with_chain_id(chain_id);
        let client = SignerMiddleware::new(provider.clone(), wallet.clone());
        let contract = contract.parse::<Address>()?;

        Ok(TxSender {
            chain_id,
            client,
            contract,
        })
    }

    /// Send a transaction with the given calldata.
    pub async fn send(&self, calldata: Vec<u8>) -> Result<Option<TransactionReceipt>> {
        let tx = TransactionRequest::new()
            .chain_id(self.chain_id)
            .to(self.contract)
            .from(self.client.address())
            .data(calldata);

        log::info!("Transaction request: {:?}", &tx);

        let tx = self.client.send_transaction(tx, None).await?.await?;

        log::info!("Transaction receipt: {:?}", &tx);

        Ok(tx)
    }
}

/// Arguments of the publisher CLI.
#[derive(Parser, Debug)]
#[clap(author, version, about, long_about = None)]
struct Args {
    /// Ethereum chain ID
    #[clap(long)]
    chain_id: u64,

    /// Ethereum Node endpoint.
    #[clap(long, env)]
    eth_wallet_private_key: String,

    /// Ethereum Node endpoint.
    #[clap(long)]
    rpc_url: String,

    /// Application's contract address on Ethereum
    #[clap(long)]
    contract: String,
}

/* This is for ARMs Remote Attestation.
/// Input data for the zkVM proof request.
#[derive(Serialize, Deserialize)]
struct InputData {
    vcek: Vec<u8>,
    cert_chain: Vec<u8>,
    public_key: Vec<u8>,
}*/

fn main() -> Result<()> {
    env_logger::init();

    let args = Args::parse();

    let tx_sender = TxSender::new(
        args.chain_id,
        &args.rpc_url,
        &args.eth_wallet_private_key,
        &args.contract,
    )?;

    let attestation_report_quote_hex =
        fs::read_to_string("./apps/data/phala_tdx_quote")
            .context("Unable to read phala_tdx_quote")?;

    let public_key_pem =
        fs::read_to_string("./apps/data/public_key.pem")
            .context("Unable to read public_key.pem")?;

    let attestation_report_quote = hex::decode(attestation_report_quote_hex.trim())
        .context("Failed to decode hex string from phala_tdx_quote")?;

    log::info!("Attestation quote size: {} bytes", attestation_report_quote.len());

    let env = ExecutorEnv::builder()
        .write(&attestation_report_quote)?
        .build()?;

    // Erst STARK-Proof generieren
    log::info!("Starting STARK proving...");
    let stark_receipt = default_prover()
        .prove_with_ctx(
            env,
            &VerifierContext::default(),
            VERIFY_AR_ELF,
            &ProverOpts::default(), // STARK
        )?
        .receipt;

    log::info!("STARK proving succeeded, converting to Groth16...");

    // STARK → Groth16 Konvertierung via HTTP-Service
    let client = reqwest::blocking::Client::new();
    let prover_url = std::env::var("RISC0_GROTH16_PROVER_URL")
        .unwrap_or_else(|_| "http://risc0-groth16-prover:8080".to_string());

    log::info!("Sending STARK receipt to Groth16 service at {}", prover_url);
    
    let stark_receipt_bytes = bincode::serialize(&stark_receipt)?;
    
    let response = client
        .post(format!("{}/prove", prover_url))
        .body(stark_receipt_bytes)
        .send()
        .context("Failed to connect to Groth16 prover service")?;

    if !response.status().is_success() {
        anyhow::bail!("Groth16 service returned error: {}", response.status());
    }

    let groth16_receipt_bytes = response.bytes()?;
    let receipt: risc0_zkvm::Receipt = bincode::deserialize(&groth16_receipt_bytes)?;

    log::info!("Groth16 proving succeeded via external service.");

    let groth16_seal = receipt
        .inner
        .groth16()
        .context("Receipt is not a Groth16 receipt")?;

    let seal_bytes = groth16::encode(groth16_seal.seal.clone())
        .context("Failed to encode Groth16 seal")?;

    let journal = receipt.journal.bytes.clone();
    let x = U256::from_be_slice(&journal);

    log::info!("Journal value x = {x}");

    let calldata = IDeviceRegistry::IDeviceRegistryCalls::registerDevice(
        IDeviceRegistry::registerDeviceCall {
            x,
            seal: seal_bytes.into(),
            _address: "0xe7f1725e7734ce288f8367e1bb143e90bb3f0512".parse().unwrap(),
            _public_ip: "0x90F79bf6EB2c4f870365E785982E1f101E93b906".to_string(),
            _msg_broker_ip: "https://7767843ff35bc83acc5cabb9b6d843672a5432f8-8090.dstack-pha-prod7.phala.network".to_string(),
            _public_key: public_key_pem.into_bytes().into(),
        },
    )
    .abi_encode();

    let runtime = tokio::runtime::Runtime::new()?;
    runtime.block_on(tx_sender.send(calldata))?;

    Ok(())
}