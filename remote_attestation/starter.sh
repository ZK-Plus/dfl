#!/bin/bash

osascript -e 'tell application "Terminal"
  do script "bash -c \"anvil\""
  activate
end tell'

echo "Anvil wurde in einem neuen Terminal gestartet."

export ETH_WALLET_PRIVATE_KEY=0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80

echo "Wallet private key wurde gesetzt." 

#cargo clean

#cargo build

export rpc_url=http://localhost:8545

forge script --rpc-url $rpc_url --broadcast script/Deploy.s.sol

export DEVICE_REGISTRY_ADDRESS=$(jq -re '.transactions[] | select(.contractName == "DeviceRegistry") | .contractAddress' ./broadcast/Deploy.s.sol/31337/run-latest.json)

export AGGREGATOR_SELECTION_ADDRESS=$(jq -re '.transactions[] | select(.contractName == "AggregatorSelection") | .contractAddress' ./broadcast/Deploy.s.sol/31337/run-latest.json)

export GMSTORAGE=$(jq -re '.transactions[] | select(.contractName == "GMStorage") | .contractAddress' ./broadcast/Deploy.s.sol/31337/run-latest.json)


export ADDRESS_1=0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266

export ADDRESS_2=0x70997970C51812dc3A010C7d01b50e0d17dc79C8

export PRIVATE_KEY_1=0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80

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

#cast call --rpc-url http://localhost:8545 $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_1

#cast call --rpc-url http://localhost:8545 $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_2

#cast send --rpc-url http://localhost:8545 --private-key $PRIVATE_KEY_1 $CONTRACT_ADDRESS "authorizeAddress(address)" $ADDRESS_2

#cast call --rpc-url http://localhost:8545 $CONTRACT_ADDRESS "isAuthorized(address)" $ADDRESS_2

##RISC0_DEV_MODE=1 cargo run -- --chain-id 31337 --eth-wallet-private-key $PRIVATE_KEY_1 --rpc-url $rpc_url --contract $DEVICE_REGISTRY_ADDRESS
#RISC0_DEV_MODE=0 cargo run -- --chain-id 31337 --eth-wallet-private-key $PRIVATE_KEY_1 --rpc-url $rpc_url --contract $DEVICE_REGISTRY_ADDRESS