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

use std::fs;
use pem::parse;
use serde::{Serialize, Deserialize};
//use tracing_subscriber;
use std::env;

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

/// Input data for the zkVM proof request.
#[derive(Serialize, Deserialize)]
struct InputData {
    vcek: Vec<u8>,
    cert_chain: Vec<u8>,
    public_key: Vec<u8>,
}

fn main() -> Result<()> {
    env_logger::init();
    // Parse CLI Arguments: The application starts by parsing command-line arguments provided by the user.
    let args = Args::parse();

    // Create a new transaction sender using the parsed arguments.
    let tx_sender = TxSender::new(
        args.chain_id,
        &args.rpc_url,
        &args.eth_wallet_private_key,
        &args.contract,
    )?;

    // Read and parse the PEM files
    let vcek_content = fs::read("./apps/data/vcek.pem").expect("Unable to read PEM file 1");
    let vcek = parse(&vcek_content).expect("Unable to parse PEM file 1");

    let cert_chain_content = fs::read("./apps/data/cert_chain.pem").expect("Unable to read PEM file 2");
    let cert_chain = parse(&cert_chain_content).expect("Unable to parse PEM file 2");

    let public_key_content = fs::read("./apps/data/public_key.pem").expect("Unable to read PEM file 3");
    let public_key = parse(&public_key_content).expect("Unable to parse PEM file 3");

    //print length of ca cert
    log::info!("CA cert length: {}", cert_chain.contents.len());
    log::info!("VCEK cert length: {}", vcek.contents.len());

    // Serialize the PEM data
    let input_data = InputData {
        vcek: vcek.contents,
        cert_chain: cert_chain.contents,
        public_key: public_key.contents.clone(),
    };

    let serialized_data = bincode::serialize(&input_data).expect("Serialization failed");
    
    // Debug: Print serialized data length
    log::info!("Serialized data length: {}", serialized_data.len());


    let env = ExecutorEnv::builder().write(&serialized_data)?.build()?;
    //let env = ExecutorEnv::builder().write_slice(&input).build()?;

    log::info!("Executing proof request...");

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

    // Encode the seal with the selector.
    let seal = groth16::encode(receipt.inner.groth16()?.seal.clone())?;

    // Extract the journal from the receipt.
    let journal = receipt.journal.bytes.clone();

    // print size of the journal
    log::info!("Journal size: {}", receipt.journal.bytes.len());

    // print size of the seal
    log::info!("Seal size: {}", seal.len());

    // Decode Journal: Upon receiving the proof, the application decodes the journal to extract
    // the verified number. This ensures that the number being submitted to the blockchain matches
    // the number that was verified off-chain.
    let x = U256::abi_decode(&journal, true).context("decoding journal data")?;

    log::info!("Verification result: {}", x);

    // calculate execution time in seconds
    let execution_time = end.duration_since(start).as_secs_f64();
    log::info!("Execution time: {:.2} seconds", execution_time);
    

    // let addr_str = "0x927507b066B40C5a6b0fd327eFE1c9d33edeE759";
    // let addr: Address = addr_str.parse().expect("Invalid address format");

    log::info!("Sending transaction to contract...");

    // Construct function call: Using the IEvenNumber interface, the application constructs
    // the ABI-encoded function call for the set function of the EvenNumber contract.
    // This call includes the verified number, the post-state digest, and the seal (proof).
    // let public_key_str = String::from_utf8_lossy(&public_key.contents).to_string();
    // let calldata = IDeviceRegistry::IDeviceRegistryCalls::registerDevice(IDeviceRegistry::registerDeviceCall {
    //     x,
    //     seal: seal.into(),
    //     _address: "0x8B088e2424b0a09b04Fc8212Bd7f0e3F38Cb2dBD".parse().unwrap(),
    //     _public_ip: "test_public_ip".to_string(),
    //     _msg_broker_ip: "test_msg_broker_ip".to_string(),
    //     _public_key: public_key.contents,
    // })
    // .abi_encode();
    let calldata = IDeviceRegistry::IDeviceRegistryCalls::runProof(IDeviceRegistry::runProofCall {
        seal: seal.into(),
        x,
    })
    .abi_encode();

    // Initialize the async runtime environment to handle the transaction sending.
    let runtime = tokio::runtime::Runtime::new()?;

    // Send transaction: Finally, the TxSender component sends the transaction to the Ethereum blockchain,
    // effectively calling the set function of the EvenNumber contract with the verified number and proof.
    runtime.block_on(tx_sender.send(calldata))?;

    Ok(())
}