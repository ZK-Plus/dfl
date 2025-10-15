# Instructions



Create a .env file in the same folder as the compose file with the following values. 

```
# Round configuration
CLIENT_LIMIT=2
EPOCH=1
ROUND=5

# IPFS
IPFS_PROVIDER=kubo
KUBO_API=http://ipfs_local:5001
KUBO_GATEWAY=http://ipfs_local:8080
INITIAL_GM_CID=QmZuFtULnQRT3xX9yQKVaPVXp54BVYNTyoueBbHCApbUr8

# Anvil deployed contract addresses
REGISTRY_ADDRESS= 0xe7f1725e7734ce288f8367e1bb143e90bb3f0512
AGGREGATOR_ADDRESS= 0x9fe46736679d2d9a65f0992f2272de9f3c7fa6e0
GM_STORAGE_ADDRESS= 0xcf7ed3acca5a467e9e704c703e8d87f634fb0fc9
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

Be sure to have Docker Desktop running.

From DFL/ then run 

```
docker compose -f docker/compose.yml up --build
```

Or, from DFL/docker/ run 

```
docker compose -f compose.yml up --build
```

Open Kubu Web UI

```
http://127.0.0.1:5001/ipfs/bafybeifplj2s3yegn7ko7tdnwpoxa4c5uaqnk2ajnw5geqm34slcj6b6mu/#/welcome
```

Check current IPFS CID in GM Contract from a new console
```
cast call "0xCf7Ed3AccA5a467e9e704C703E8D87F634fB0Fc9" "globalModel()(string)" --rpc-url http://localhost:8545
```