#!/bin/bash


export ETH_WALLET_PRIVATE_KEY=0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80

echo "Wallet private key wurde gesetzt." 

#cargo clean

#cargo build

export rpc_url=http://anvil:8545

forge script --rpc-url $rpc_url --broadcast script/Deploy.s.sol

export DEVICE_REGISTRY_ADDRESS=$(jq -re '.transactions[] | select(.contractName == "DeviceRegistry") | .contractAddress' ./broadcast/Deploy.s.sol/31337/run-latest.json)

export AGGREGATOR_SELECTION_ADDRESS=$(jq -re '.transactions[] | select(.contractName == "AggregatorSelection") | .contractAddress' ./broadcast/Deploy.s.sol/31337/run-latest.json)

export GMSTORAGE=$(jq -re '.transactions[] | select(.contractName == "GMStorage") | .contractAddress' ./broadcast/Deploy.s.sol/31337/run-latest.json)


export ADDRESS_1=0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266

export ADDRESS_2=0x70997970C51812dc3A010C7d01b50e0d17dc79C8

export ADDRESS_3=0x3C44CdDdB6a900fa2b585dd299e03d12FA4293BC

export ADDRESS_4=0x90F79bf6EB2c4f870365E785982E1f101E93b906

export PRIVATE_KEY_1=0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80

export PRIVATE_KEY_2=0x59c6995e998f97a5a0044966f0945389dc9e86dae88c7a8412f4603b6b78690d

export PRIVATE_KEY_3=0x5de4111afa1a4b94908f83103eb1f1706367c2e68ca870fc3fb9a804cdab365a

export PRIVATE_KEY_4=0x7c852118294e51e653712a81e05800f419141751be58f605c371e15141b007a6

echo "========= node server environment variables ========="

echo "REGISTRY_ADDRESS= $DEVICE_REGISTRY_ADDRESS" 

echo "AGGREGATOR_ADDRESS= $AGGREGATOR_SELECTION_ADDRESS" 

echo "GM_STORAGE_ADDRESS= $GMSTORAGE"

echo "ACCOUNT_ADDRESS= $ADDRESS_1"

echo "PRIVATE_KEY= $PRIVATE_KEY_1"

echo "SEPOLIA_RPC_URL= $rpc_url"

echo "===================================================="

cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_1 $AGGREGATOR_SELECTION_ADDRESS "setGMStorageAddress(address)" $GMSTORAGE

echo "GMStorage Addresse wurde in AggregatorSelection gesetzt"


echo "Authorization Status:"
[ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_1)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_1: yes || echo $ADDRESS_1: no
[ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_2)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_2: yes || echo $ADDRESS_2: no
[ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_3)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_3: yes || echo $ADDRESS_3: no
[ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_4)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_4: yes || echo $ADDRESS_4: no


echo "Authorized Workers 2 to 4"
cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_1 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_2
cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_1 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_3
cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_1 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_4

echo "Authorization Status:"
[ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_1)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_1: yes || echo $ADDRESS_1: no
[ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_2)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_2: yes || echo $ADDRESS_2: no
[ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_3)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_3: yes || echo $ADDRESS_3: no
[ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_4)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_4: yes || echo $ADDRESS_4: no


#RISC0_DEV_MODE=1 cargo run -- --chain-id 31337 --eth-wallet-private-key $PRIVATE_KEY_1 --rpc-url $rpc_url --contract $DEVICE_REGISTRY_ADDRESS
#RISC0_DEV_MODE=0 cargo run -- --chain-id 31337 --eth-wallet-private-key $PRIVATE_KEY_1 --rpc-url $rpc_url --contract $DEVICE_REGISTRY_ADDRESS