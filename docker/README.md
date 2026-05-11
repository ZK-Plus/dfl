
# How to use the MVP with Docker

You need atleast a .env File as showen below and the [compose](.compose.yml) file. You can clone the DFL project as well and compile the Docker images yourself. 

## Manditory .env file

Create a .env file in the same folder as the compose file with the following values. 

```
# Round configuration
CLIENT_LIMIT=2
MODEL_SUBMISSION_DEADLINE_MS=60000
GM_UPDATE_TIMEOUT_MS=30000
GM_UPDATE_TIMEOUT_LOOPS=4
AGGREGATION_UPDATE_ESTIMATE_MS=30000
GM_UPDATE_POLL_MS=5000
AGGREGATOR_TIMEOUT_REPORT_PERCENT=50
EPOCH=1
ROUND=5
#DOCKER=phala
DOCKER=local

# IPFS
IPFS_PROVIDER=kubo
KUBO_API=http://ipfs_local:5001
KUBO_GATEWAY=http://ipfs_local:8080
INITIAL_GM_CID=QmZuFtULnQRT3xX9yQKVaPVXp54BVYNTyoueBbHCApbUr8

# Anvil deployed contract addresses
REGISTRY_ADDRESS=0x5FbDB2315678afecb367f032d93F642f64180aa3
AGGREGATOR_ADDRESS=0xe7f1725E7734CE288F8367e1Bb143E90bb3F0512
GM_STORAGE_ADDRESS=0x9fE46736679d2D9a65F0992F2272dE9f3c7fa6e0
RPC_URL= http://anvil:8545

# Worker 0
W0_ACCOUNT_ADDRESS= 0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266
W0_PRIVATE_KEY= 0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80
W0_DEVICE_ID= 0

# Worker 1
W1_ACCOUNT_ADDRESS= 0x70997970C51812dc3A010C7d01b50e0d17dc79C8
W1_PRIVATE_KEY=0x59c6995e998f97a5a0044966f0945389dc9e86dae88c7a8412f4603b6b78690d
W1_DEVICE_ID= 1

# Worker 2
W2_ACCOUNT_ADDRESS= 0x3C44CdDdB6a900fa2b585dd299e03d12FA4293BC
W2_PRIVATE_KEY= 0x5de4111afa1a4b94908f83103eb1f1706367c2e68ca870fc3fb9a804cdab365a
W2_DEVICE_ID= 2

# Worker 3
W3_ACCOUNT_ADDRESS= 0x90F79bf6EB2c4f870365E785982E1f101E93b906
W3_PRIVATE_KEY= 0x7c852118294e51e653712a81e05800f419141751be58f605c371e15141b007a6
W3_DEVICE_ID= 3

```

# Overview of the Docker files

## The main compose file

For denmonstration pourposes we recomend the main [compose](.compose.yml) file. It will start the following services.

- A local Blockchain (Anvil with 20 Accounts)
- A local IPFS Service (Kubo)
- Deploy the Smart contracts (Foundry in the Remote Attestation Project) 
- Workers 0 to 2

Be sure to have Docker Desktop running.

Either clone the DFL project if you want to compile the files yourself and from DFL/ then run 

```
docker compose -f docker/compose.yml up --build
```

or just copy the main [compose](.compose.yml) file and create the .env file in the same folder. 

## The remote attestation compose file

It will start the following services.

- Deploy the Smart contracts (Foundry in the Remote Attestation Project) 
- Generate the SNARK (not supported).

This [Remote Attestation Compose](.compose_ra.yml) file is for smart contract deployment by the foundry framework or for test purposes, since the STARK to SNARK conversation only works on x86 Machines not inside Docker Containers.

Be sure to have Docker Desktop running.

Either clone the DFL project if you want to compile the files yourself and from DFL/ then run 

```
docker compose -f docker/compose_ra.yml up --build
```

## The compiler compose file

This [Compiler Compose](.compiler_compose.yml) file builds the neural network code .exe file, since it is written in c++.

Be sure to have Docker Desktop running.

Clone the DFL project first and from DFL/ then run 

```
docker compose -f docker/compiler_compose.yml up --build
```



# Optional 

If you wish to use [pinata](https://pinata.cloud/) instead of a local [Kubo](https://github.com/ipfs/kubo) IPFS node replace the dummy credentials with your own. Upload the initial global model in IPFS usind the Add function in FILES from [pinata](https://pinata.cloud/). Choose FILE UPLOAD and the Initial_GM file in this folder, then repace the INITIAL_GM_CID with your new CID. 

```
# IPFS Pinata credentials
API_KEY= 03xxxxx...
API_SECRET= 089xxxx...
PINATA_JWT=eyJhbxxxxxx...
IPFS_GATEWAY= https://xxxxx.....xxxxxx.mypinata.cloud
INITIAL_GM_CID=bafxxxxx...
IPFS_PROVIDER=pinata
```

## Timing variables

`MODEL_SUBMISSION_DEADLINE_MS` controls how long the aggregator waits for model submissions before aggregating with the files that arrived.

`GM_UPDATE_TIMEOUT_MS * GM_UPDATE_TIMEOUT_LOOPS` is the total worker-side wait budget before workers report an aggregator timeout on-chain. As a starting point, keep it above:

```
MODEL_SUBMISSION_DEADLINE_MS + AGGREGATION_UPDATE_ESTIMATE_MS + GM_UPDATE_POLL_MS
```

With a 50% timeout report threshold, 1 of 2 eligible non-aggregator workers is enough to abort a stalled round. Use 51% when both workers should have to agree in a two-worker setup.

## Observability

The Python compose setup starts a local observability stack:

- Grafana: http://127.0.0.1:3000
- Prometheus: http://127.0.0.1:9090
- Loki: http://127.0.0.1:3100
- Tempo: http://127.0.0.1:3200
- OpenTelemetry Collector OTLP HTTP: http://127.0.0.1:4318

Grafana is provisioned with Prometheus, Loki and Tempo datasources plus the `DFL Training Overview` dashboard. Node workers export one trace per training round. Each trace contains spans for fetching the global model, local training, model transfer, aggregation, update, penalties and aggregator selection.

In Grafana, open `Explore`, choose the `Tempo` datasource and search with:

```
{ resource.service.name = "dfl-node-worker" }
```

The `dfl.round`, `dfl.role`, `dfl.account` and `dfl.duration_ms` attributes can be used to inspect one training round across all participating nodes.

## Helpfull commands

Open Kubu Web UI

```
http://127.0.0.1:5001/ipfs/bafybeifplj2s3yegn7ko7tdnwpoxa4c5uaqnk2ajnw5geqm34slcj6b6mu/#/welcome
```

Check current IPFS CID in GM Contract from a new console
```
cast call "0xCf7Ed3AccA5a467e9e704C703E8D87F634fB0Fc9" "globalModel()(string)" --rpc-url http://localhost:8545
```


Build a local Docker image

```
docker build --platform linux/amd64 -f docker/Dockerfile -t docker_hub_nickname/project_name:latest .
```

Push the local Docker image to Docker Hub

```
docker push docker_hub_nickname/project_name:latest && docker push docker_hub_nickname/project_name:latest
```

Delete all Docker files

```
docker system prune -a --volumes
```
