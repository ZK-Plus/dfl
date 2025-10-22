#!/bin/bash

osascript -e 'tell application "Terminal"
  do script "bash -c \"anvil\""
  activate
end tell'

echo "Anvil wurde in einem neuen Terminal gestartet."

sleep 5

export ETH_WALLET_PRIVATE_KEY=0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80

echo "Wallet private key wurde gesetzt." 

#cargo clean

#cargo build

export rpc_url=http://localhost:8545

forge script --rpc-url $rpc_url --broadcast script/Deploy.s.sol

export DEVICE_REGISTRY_ADDRESS=$(jq -re '.transactions[] | select(.contractName == "DeviceRegistry") | .contractAddress' ./broadcast/Deploy.s.sol/31337/run-latest.json)

export AGGREGATOR_SELECTION_ADDRESS=$(jq -re '.transactions[] | select(.contractName == "AggregatorSelection") | .contractAddress' ./broadcast/Deploy.s.sol/31337/run-latest.json)

export GMSTORAGE=$(jq -re '.transactions[] | select(.contractName == "GMStorage") | .contractAddress' ./broadcast/Deploy.s.sol/31337/run-latest.json)


export ADDRESS_0=0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266
export ADDRESS_1=0x70997970C51812dc3A010C7d01b50e0d17dc79C8
export ADDRESS_2=0x3C44CdDdB6a900fa2b585dd299e03d12FA4293BC
export ADDRESS_3=0x90F79bf6EB2c4f870365E785982E1f101E93b906
export ADDRESS_4=0x15d34AAf54267DB7D7c367839AAf71A00a2C6A65
export ADDRESS_5=0x9965507D1a55bcC2695C58ba16FB37d819B0A4dc
export ADDRESS_6=0x976EA74026E726554dB657fA54763abd0C3a0aa9
export ADDRESS_7=0x14dC79964da2C08b23698B3D3cc7Ca32193d9955
export ADDRESS_8=0x23618e81E3f5cdF7f54C3d65f7FBc0aBf5B21E8f
export ADDRESS_9=0xa0Ee7A142d267C1f36714E4a8F75612F20a79720
export ADDRESS_10=0xBcd4042DE499D14e55001CcbB24a551F3b954096
export ADDRESS_11=0x71bE63f3384f5fb98995898A86B02Fb2426c5788
export ADDRESS_12=0xFABB0ac9d68B0B445fB7357272Ff202C5651694a
export ADDRESS_13=0x1CBd3b2770909D4e10f157cABC84C7264073C9Ec
export ADDRESS_14=0xdF3e18d64BC6A983f673Ab319CCaE4f1a57C7097
export ADDRESS_15=0xcd3B766CCDd6AE721141F452C550Ca635964ce71
export ADDRESS_16=0x2546BcD3c84621e976D8185a91A922aE77ECEc30
export ADDRESS_17=0xbDA5747bFD65F08deb54cb465eB87D40e51B197E
export ADDRESS_18=0xdD2FD4581271e230360230F9337D5c0430Bf44C0
export ADDRESS_19=0x8626f6940E2eb28930eFb4CeF49B2d1F2C9C1199

export PRIVATE_KEY_0=0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80
export PRIVATE_KEY_1=0x59c6995e998f97a5a0044966f0945389dc9e86dae88c7a8412f4603b6b78690d
export PRIVATE_KEY_2=0x5de4111afa1a4b94908f83103eb1f1706367c2e68ca870fc3fb9a804cdab365a
export PRIVATE_KEY_3=0x7c852118294e51e653712a81e05800f419141751be58f605c371e15141b007a6
export PRIVATE_KEY_4=0x47e179ec197488593b187f80a00eb0da91f1b9d0b13f8733639f19c30a34926a
export PRIVATE_KEY_5=0x8b3a350cf5c34c9194ca85829a2df0ec3153be0318b5e2d3348e872092edffba
export PRIVATE_KEY_6=0x92db14e403b83dfe3df233f83dfa3a0d7096f21ca9b0d6d6b8d88b2b4ec1564e
export PRIVATE_KEY_7=0x4bbbf85ce3377467afe5d46f804f221813b2bb87f24d81f60f1fcdbf7cbf4356
export PRIVATE_KEY_8=0xdbda1821b80551c9d65939329250298aa3472ba22feea921c0cf5d620ea67b97
export PRIVATE_KEY_9=0x2a871d0798f97d79848a013d4936a73bf4cc922c825d33c1cf7073dff6d409c6
export PRIVATE_KEY_10=0xf214f2b2cd398c806f84e317254e0f0b801d0643303237d97a22a48e01628897
export PRIVATE_KEY_11=0x701b615bbdfb9de65240bc28bd21bbc0d996645a3dd57e7b12bc2bdf6f192c82
export PRIVATE_KEY_12=0xa267530f49f8280200edf313ee7af6b827f2a8bce2897751d06a843f644967b1
export PRIVATE_KEY_13=0x47c99abed3324a2707c28affff1267e45918ec8c3f20b8aa892e8b065d2942dd
export PRIVATE_KEY_14=0xc526ee95bf44d8fc405a158bb884d9d1238d99f0612e9f33d006bb0789009aaa
export PRIVATE_KEY_15=0x8166f546bab6da521a8369cab06c5d2b9e46670292d85c875ee9ec20e84ffb61
export PRIVATE_KEY_16=0xea6c44ac03bff858b476bba40716402b03e41b8e97e276d1baec7c37d42484a0
export PRIVATE_KEY_17=0x689af8efa8c651a91ad287602527f3af2fe9f6501a7ac4b061667b5a93e037fd
export PRIVATE_KEY_18=0xde9be858da4a475276426320d5e9262ecfc3ba460bfac56360bfa6c4c28b4ee0
export PRIVATE_KEY_19=0xdf57089febbacf7ba0bc227dafbffa9fc08a93fdc68e1e42411a14efcf23656e


echo "========= node server environment variables ========="

echo "REGISTRY_ADDRESS= $DEVICE_REGISTRY_ADDRESS" 

echo "AGGREGATOR_ADDRESS= $AGGREGATOR_SELECTION_ADDRESS" 

echo "GM_STORAGE_ADDRESS= $GMSTORAGE"

echo "ACCOUNT_ADDRESS= $ADDRESS_0"

echo "PRIVATE_KEY= $PRIVATE_KEY_0"

echo "SEPOLIA_RPC_URL= $rpc_url"

echo "===================================================="

cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $AGGREGATOR_SELECTION_ADDRESS "setGMStorageAddress(address)" $GMSTORAGE

echo "GMStorage Addresse wurde in AggregatorSelection gesetzt"


# echo "Authorization Status:"
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_0)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_0: yes || echo $ADDRESS_0: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_1)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_1: yes || echo $ADDRESS_1: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_2)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_2: yes || echo $ADDRESS_2: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_3)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_3: yes || echo $ADDRESS_3: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_4)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_4: yes || echo $ADDRESS_4: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_5)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_5: yes || echo $ADDRESS_5: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_6)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_6: yes || echo $ADDRESS_6: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_7)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_7: yes || echo $ADDRESS_7: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_8)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_8: yes || echo $ADDRESS_8: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_9)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_9: yes || echo $ADDRESS_9: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_10)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_10: yes || echo $ADDRESS_10: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_11)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_11: yes || echo $ADDRESS_11: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_12)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_12: yes || echo $ADDRESS_12: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_13)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_13: yes || echo $ADDRESS_13: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_14)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_14: yes || echo $ADDRESS_14: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_15)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_15: yes || echo $ADDRESS_15: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_16)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_16: yes || echo $ADDRESS_16: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_17)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_17: yes || echo $ADDRESS_17: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_18)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_18: yes || echo $ADDRESS_18: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_19)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_19: yes || echo $ADDRESS_19: no

# echo "Authorized Workers 0 to 19"
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_0
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_1
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_2
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_3
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_4
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_5
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_6
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_7
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_8
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_9
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_10
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_11
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_12
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_13
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_14
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_15
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_16
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_17
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_18
# cast send --rpc-url $rpc_url --private-key $PRIVATE_KEY_0 $DEVICE_REGISTRY_ADDRESS "authorizeAddress(address)" $ADDRESS_19

# echo "Authorization Status:"
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_0)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_0: yes || echo $ADDRESS_0: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_1)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_1: yes || echo $ADDRESS_1: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_2)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_2: yes || echo $ADDRESS_2: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_3)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_3: yes || echo $ADDRESS_3: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_4)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_4: yes || echo $ADDRESS_4: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_5)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_5: yes || echo $ADDRESS_5: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_6)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_6: yes || echo $ADDRESS_6: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_7)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_7: yes || echo $ADDRESS_7: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_8)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_8: yes || echo $ADDRESS_8: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_9)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_9: yes || echo $ADDRESS_9: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_10)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_10: yes || echo $ADDRESS_10: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_11)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_11: yes || echo $ADDRESS_11: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_12)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_12: yes || echo $ADDRESS_12: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_13)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_13: yes || echo $ADDRESS_13: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_14)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_14: yes || echo $ADDRESS_14: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_15)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_15: yes || echo $ADDRESS_15: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_16)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_16: yes || echo $ADDRESS_16: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_17)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_17: yes || echo $ADDRESS_17: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_18)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_18: yes || echo $ADDRESS_18: no
# [ "$(cast call --rpc-url $rpc_url $DEVICE_REGISTRY_ADDRESS "isAuthorized(address)" $ADDRESS_19)" = "0x$(printf '%063d1')" ] && echo $ADDRESS_19: yes || echo $ADDRESS_19: no



#RISC0_DEV_MODE=1 cargo run -- --chain-id 31337 --eth-wallet-private-key $PRIVATE_KEY_1 --rpc-url $rpc_url --contract $DEVICE_REGISTRY_ADDRESS
RISC0_DEV_MODE=0 cargo run -- --chain-id 31337 --eth-wallet-private-key $PRIVATE_KEY_1 --rpc-url $rpc_url --contract $DEVICE_REGISTRY_ADDRESS