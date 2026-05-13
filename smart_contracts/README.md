# smart_contracts

This directory is a focused Foundry deployment bundle extracted from
`smart_contract/` and `remote_attestation/`.

Included here:

- core contracts required by the DFL pipeline:
  - `src/core/GMStorage.sol`
  - `src/core/AggregatorSelection.sol`
  - `src/core/DeviceRegistry.sol`
- TDX/DCAP attestation contracts used by the deployment flow:
  - `src/attestation/AutomataDcapTdxV4Attestation.sol`
  - `src/attestation/tdx/QuoteV4Auth/*`
- deployment and verification scripts:
  - `script/Deploy.s.sol`
  - `script/DeployTDXV4Attestation.s.sol`
  - `script/ProbeP256Verifier.s.sol`
  - `script/UploadPccsCollaterals.s.sol`
  - `script/VerifyTDXV4Quote.s.sol`
  - `starter_docker.sh`
- runtime quote asset:
  - `data/phala_tdx_quote`
- Solidity dependencies needed by those scripts and contracts:
  - `lib/forge-std`
  - `lib/openzeppelin-contracts/contracts`
  - `lib/p256-verifier/src`
  - `lib/automata-dcap-v3-attestation/contracts`
  - `lib/automata-dcap-v3-attestation/lib/automata-on-chain-pccs`

Intentionally not copied:

- Rust guest/method code
- publisher app sources
- tests, images, docs, and generated `out/` artifacts from `remote_attestation`
- unrelated smart contract examples and legacy build outputs

The Docker smart-contract services now use this directory as the primary source.
