import Web3 from "web3";
import fs from "fs";
// setup client´
const web3 = new Web3("https://eth-sepolia.g.alchemy.com/v2/pFowzUSGYob62Q7i2YVsF0LFUX3WiCT2");
// define smart contract adddresses
const gm_storage_address = "0x2F2C7B8B173A5F12d606dF79D15FB030d3886F80";
const device_registry_address = "";
const privateKey = "0xcf285e00a7b51bf21f3f998590a0f3f374d533c43e5e88306148deeb4898fa0b";
// send contract call to blockchain
export const getCurrentGM = async () => {
    // get contract abi from json file using fs instead of require
    const abi = JSON.parse(fs.readFileSync("./abi/gm.json", "utf-8"));
    // get contract address
    const address = gm_storage_address;
    // get contract
    const contract = new web3.eth.Contract(abi, address);
    // call method of contract
    let ipfs_address = await contract.methods.getGlobalModel().call().then((result) => {
        return result;
    });
    return ipfs_address;
};
// function to set global model
export const setGlobalModel = async (newIpfsAddress) => {
    const abi = JSON.parse(fs.readFileSync("./abi/gm.json", "utf-8"));
    const address = gm_storage_address;
    const contract = new web3.eth.Contract(abi, address);
    const account = web3.eth.accounts.privateKeyToAccount(privateKey);
    web3.eth.accounts.wallet.add(account);
    const gasPrice = await web3.eth.getGasPrice();
    const gasEstimate = await contract.methods.setGlobalModel(newIpfsAddress).estimateGas({ from: account.address });
    const tx = {
        from: account.address,
        to: address,
        gas: gasEstimate,
        gasPrice: gasPrice,
        data: contract.methods.setGlobalModel(newIpfsAddress).encodeABI(),
    };
    try {
        const signedTx = await web3.eth.accounts.signTransaction(tx, privateKey);
        const receipt = await web3.eth.sendSignedTransaction(signedTx.rawTransaction);
        console.log("Transaction receipt: ", receipt);
        return receipt;
    }
    catch (error) {
        console.error("Error sending transaction: ", error);
        throw error;
    }
};
