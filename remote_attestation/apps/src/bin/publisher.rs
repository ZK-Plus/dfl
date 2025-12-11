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
use alloy_sol_types::{sol, SolInterface, SolValue};
use anyhow::{Context, Result};
use clap::Parser;
use ethers::prelude::*;
use methods::VERIFY_AR_ELF;
use risc0_ethereum_contracts::groth16;
use risc0_zkvm::{default_prover, ExecutorEnv, ProverOpts, VerifierContext};
use hex;
use std::fs;

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

pub struct TxSender {
    chain_id: u64,
    client: SignerMiddleware<Provider<Http>, Wallet<k256::ecdsa::SigningKey>>,
    contract: Address,
}

impl TxSender {
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

#[derive(Parser, Debug)]
#[clap(author, version, about, long_about = None)]
struct Args {
    #[clap(long)]
    chain_id: u64,

    #[clap(long, env)]
    eth_wallet_private_key: String,

    #[clap(long)]
    rpc_url: String,

    #[clap(long)]
    contract: String,
}

fn main() -> Result<()> {
    env_logger::init();
    let args = Args::parse();

    let tx_sender = TxSender::new(
        args.chain_id,
        &args.rpc_url,
        &args.eth_wallet_private_key,
        &args.contract,
    )?;

    let attestation_report_quote_hex = fs::read_to_string("./apps/data/phala_tdx_quote").expect("Unable to read phala_tdx_quote");
    let public_key = fs::read_to_string("./apps/data/public_key.pem").expect("Unable to read public_key.pem");

    let attestation_report_quote = hex::decode(attestation_report_quote_hex.trim()).expect("Failed to decode hex string");

    let env = ExecutorEnv::builder()
        .write(&attestation_report_quote)?
        .stdout(std::io::stdout())
        .stderr(std::io::stderr())
        .build()?;

    log::info!("Executing proof request...");

    // Reduce memory usage by forcing single-threaded proving
    //std::env::set_var("RAYON_NUM_THREADS", "1");

    let start = std::time::Instant::now();


    let receipt = default_prover()
        .prove_with_ctx(
            env,
            &VerifierContext::default(),
            VERIFY_AR_ELF,
            &ProverOpts::groth16(),
        )?
        .receipt;

    log::info!("Proof request executed successfully.");
    let end = std::time::Instant::now();

    let seal = groth16::encode(receipt.inner.groth16()?.seal.clone())?;

    let journal = receipt.journal.bytes.clone();

    log::info!("Journal size: {}", receipt.journal.bytes.len());
    log::info!("Seal size: {}", seal.len());

    let x = U256::abi_decode(&journal, true).context("decoding journal data")?;

    log::info!("Verification result: {}", x);

    let execution_time = end.duration_since(start).as_secs_f64();
    log::info!("Execution time: {:.2} seconds", execution_time);

    log::info!("Sending transaction to contract...");


    let calldata = IDeviceRegistry::IDeviceRegistryCalls::registerDevice(
        IDeviceRegistry::registerDeviceCall {
            x,
            seal: seal.into(), // was seal_bytes.into()
            _address: "0xe7f1725e7734ce288f8367e1bb143e90bb3f0512".parse().unwrap(),
            _public_ip: "0x90F79bf6EB2c4f870365E785982E1f101E93b906".to_string(),
            _msg_broker_ip: "https://7767843ff35bc83acc5cabb9b6d843672a5432f8-8090.dstack-pha-prod7.phala.network".to_string(),
            _public_key: public_key.into_bytes().into(),
    )
    .abi_encode();
    
    //let calldata = IDeviceRegistry::IDeviceRegistryCalls::runProof(IDeviceRegistry::runProofCall {
    //    seal: seal.into(),
    //    x,
    //})
    //.abi_encode();

    let runtime = tokio::runtime::Runtime::new()?;

    runtime.block_on(tx_sender.send(calldata))?;

    Ok(())
}