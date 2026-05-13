# Local LangChain Agent for IPFS + ZK Inference

This folder adds an agent layer around the existing thesis pipeline:

1. The agent reads the current model CID, signature CID, and last aggregator from the `GMStorage` smart contract.
2. Both artifacts are fetched from the local IPFS node.
3. The last aggregator's public key is read from `DeviceRegistry`.
4. The downloaded model signature is verified with RSA-SHA256/PKCS1v15.
5. A local MCP server exposes `zk_inference` as tools.
6. The agent exports the verified model, creates one single-image MNIST query, runs EZKL, and returns the prediction plus proof artifact.

## Install

From the repository root:

```bash
pip install -r zk_inference/requirements.txt
pip install -e neural_network
pip install -r agent/requirements.txt
```

If you only want the deterministic local pipeline, `agent/requirements.txt` is not needed. It is only required for LangChain LLM mode and the MCP Python package.

## Configure IPFS

Use the IPFS API URL, not the browser web UI URL:

```bash
export IPFS_API_URL=http://127.0.0.1:5001
export IPFS_ROOT=/
```

For an immutable DAG/CID path, use:

```bash
export IPFS_ROOT=/ipfs/bafybeifplj2s3yegn7ko7tdnwpoxa4c5uaqnk2ajnw5geqm34slcj6b6mu
```

The URL you pasted opens the IPFS Web UI. The agent talks to the same daemon through `/api/v0/...`.

## Configure Blockchain Source

By default the agent uses the smart contract as the source of truth:

```bash
export RPC_URL=http://127.0.0.1:8545
export GM_STORAGE_ADDRESS=0x9fE46736679d2D9a65F0992F2272dE9f3c7fa6e0
export REGISTRY_ADDRESS=0x5FbDB2315678afecb367f032d93F642f64180aa3
```

If you omit these, the agent tries to read them from `docker/.env`. When `docker/.env` contains Docker-internal `http://anvil:8545`, the local agent maps it to `http://127.0.0.1:8545`.

Fetch only the on-chain model bundle:

```bash
.venv/bin/python agent/blockchain_source.py \
  --rpc-url http://127.0.0.1:8545 \
  --gm-storage-address 0x9fE46736679d2D9a65F0992F2272dE9f3c7fa6e0 \
  --registry-address 0x5FbDB2315678afecb367f032d93F642f64180aa3 \
  --ipfs-api-url http://127.0.0.1:5001 \
  --fetch \
  --verify
```

`--verify` reads `getLastRoundsAggregator()` from `GMStorage`, fetches that address's public key from `DeviceRegistry`, and verifies the raw `.sig` over the raw model bytes before any inference step.

## Fast Local Run

This fetches the latest model bundle and runs the whole proof pipeline without an LLM:

```bash
.venv/bin/python agent/run_agent.py \
  --ipfs-api-url http://127.0.0.1:5001 \
  --rpc-url http://127.0.0.1:8545 \
  --registry-address 0x5FbDB2315678afecb367f032d93F642f64180aa3 \
  --skip-calibration
```

`--source contract` is the default. For debugging old local files you can still use `--source ipfs-scan --ipfs-root /`.

In `--source ipfs-scan` mode the IPFS selector uses the newest complete pair of model plus signature. It first tries an exact filename match like `model.bin` plus `model.bin.sig`; if your training writes model and signature with slightly different timestamps, it pairs the nearest signature within `120` seconds. This matches artifacts like:

```text
2026-05-12T06-27-14-781Z-aggregated.bin
2026-05-12T06-27-14-797Z-aggregated.bin.sig
```

To change the pairing window:

```bash
--pair-window-seconds 10
```

To fail when the newest model has no pair instead, add:

```bash
--require-latest-model
```

Use a specific MNIST test image:

```bash
.venv/bin/python agent/run_agent.py --index 7 --skip-calibration
```

Output includes:

- fetched model path in `agent/downloads`
- fetched signature path in `agent/downloads`
- selected/predicted label from `zk_inference/single_query/prediction.json`
- proof path, normally `zk_inference/out/proof.json`

## LangChain LLM Mode

Start Ollama locally and make sure the model is available:

```bash
ollama pull gemma4:e2b
ollama serve
```

Then run:

```bash
export OLLAMA_BASE_URL=http://127.0.0.1:11434
export OLLAMA_MODEL=gemma4:e2b

.venv/bin/python agent/run_agent.py --llm --skip-calibration
```

No OpenAI API key is needed in this mode.

In this mode LangChain gets two local IPFS/RAG tools and the ZK tools through MCP:

- `rag_search_ipfs_models`
- `fetch_latest_model_bundle`
- `export_model`
- `create_single_image_query`
- `run_ezkl`
- `prove_single_image`

## Run the MCP Server Directly

```bash
.venv/bin/python agent/zk_mcp_server.py
```

External MCP clients can start this server over stdio. The server does not reimplement ZK logic; it calls the existing scripts in `zk_inference`.

## Notes

- `--skip-calibration` is the pragmatic default for local debugging.
- The signature is fetched and copied next to the proof artifacts, but cryptographic signature verification is not performed in this folder yet.
- If your latest model is not in IPFS MFS root, point `IPFS_ROOT` at the exact MFS folder or `/ipfs/<cid>` DAG path.
