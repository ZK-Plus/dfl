import Web3 from "web3";
import fs from "fs";
import 'dotenv/config'

// setup client´
//const web3 = new Web3("https://eth-sepolia.g.alchemy.com/v2/pFowzUSGYob62Q7i2YVsF0LFUX3WiCT2");
const web3 = new Web3(process.env.SEPOLIA_RPC_URL);


// define smart contract addresses
const gm_storage_address: string = process.env.GM_STORAGE_ADDRESS as string;
const aggregator_address: string = process.env.AGGREGATOR_ADDRESS as string;
const device_registry_address: string = process.env.REGISTRY_ADDRESS as string;

const privateKey: string = process.env.PRIVATE_KEY as string;



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
  }


// function to set global model
export const setGlobalModel = async (newIpfsAddress: string) => {
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
        const receipt = await web3.eth.sendSignedTransaction(signedTx.rawTransaction as string);
        console.log("Transaction receipt: ", receipt);
        return receipt;
    } catch (error) {
        console.error("Error sending transaction: ", error);
        throw error;
    }
}

// set the contribution of the devices
export const setContribution = async (deviceID: string[]) => {
    const abi = JSON.parse(fs.readFileSync("./abi/gm.json", "utf-8"));
    const address = gm_storage_address;
    const contract = new web3.eth.Contract(abi, address);

    const account = web3.eth.accounts.privateKeyToAccount(privateKey);
    web3.eth.accounts.wallet.add(account);

    const gasPrice = await web3.eth.getGasPrice();
    const gasEstimate = await contract.methods.incrementContribution(deviceID).estimateGas({ from: account.address });

    const tx = {
        from: account.address,
        to: address,
        gas: gasEstimate,
        gasPrice: gasPrice,
        data: contract.methods.incrementContribution(deviceID).encodeABI(),
    };

    try {
        const signedTx = await web3.eth.accounts.signTransaction(tx, privateKey);
        const receipt = await web3.eth.sendSignedTransaction(signedTx.rawTransaction as string);
        console.log("Transaction receipt: ", receipt);
        return receipt;
    } catch (error) {
        console.error("Error sending transaction: ", error);
        throw error;
    }
}

export const getTopContributor = async () => {
    const abi = JSON.parse(fs.readFileSync("./abi/gm.json", "utf-8"));
    const address = gm_storage_address;
    const contract = new web3.eth.Contract(abi, address);
    let topContributor = await contract.methods.getTopContributor().call().then((result) => {
        return result;
    });
    return topContributor;
}



// get current state from aggregator
export const getCurrentState = async () => {
    const abi = JSON.parse(fs.readFileSync("./abi/AggregatorSelection.json", "utf-8"));
    const address = aggregator_address;
    const contract = new web3.eth.Contract(abi, address);
    let state = await contract.methods.getSystemState().call().then((result) => {
        return result;
    });
    return state;
}

export const setCurrentState = async (newState: string) => {
    const abi = JSON.parse(fs.readFileSync("./abi/AggregatorSelection.json", "utf-8"));
    const address = aggregator_address;
    const contract = new web3.eth.Contract(abi, address);

    const account = web3.eth.accounts.privateKeyToAccount(privateKey);
    web3.eth.accounts.wallet.add(account);

    const gasPrice = await web3.eth.getGasPrice();
    const gasEstimate = await contract.methods.setSystemState(newState).estimateGas({ from: account.address });

    const tx = {
        from: account.address,
        to: address,
        gas: gasEstimate,
        gasPrice: gasPrice,
        data: contract.methods.setSystemState(newState).encodeABI(),
    };

    try {
        const signedTx = await web3.eth.accounts.signTransaction(tx, privateKey);
        const receipt = await web3.eth.sendSignedTransaction(signedTx.rawTransaction as string);
        console.log("Transaction receipt: ", receipt);
        return receipt;
    } catch (error) {
        console.error("Error sending transaction: ", error);
        throw error;
    }
}

/* ------------------- Helper functions for state contract ------------------ */

export const getAggregatorEndpoint = async () => {
    const abi = JSON.parse(fs.readFileSync("./abi/AggregatorSelection.json", "utf-8"));
    const address = aggregator_address;
    const contract = new web3.eth.Contract(abi, address);
    let endpoint = await contract.methods.getBrokerEndpoint().call().then((result) => {
        return result;
    });
    return endpoint;
}

export const setAggregatorEndpoint = async (newEndpoint: string) => {
    const abi = JSON.parse(fs.readFileSync("./abi/AggregatorSelection.json", "utf-8"));
    const address = aggregator_address;
    const contract = new web3.eth.Contract(abi, address);

    const account = web3.eth.accounts.privateKeyToAccount(privateKey);
    web3.eth.accounts.wallet.add(account);

    const gasPrice = await web3.eth.getGasPrice();
    const gasEstimate = await contract.methods.setBrokerEndpoint(newEndpoint).estimateGas({ from: account.address });

    const tx = {
        from: account.address,
        to: address,
        gas: gasEstimate,
        gasPrice: gasPrice,
        data: contract.methods.setBrokerEndpoint(newEndpoint).encodeABI(),
    };

    try {
        const signedTx = await web3.eth.accounts.signTransaction(tx, privateKey);
        const receipt = await web3.eth.sendSignedTransaction(signedTx.rawTransaction as string);
        console.log("Transaction receipt: ", receipt);
        return receipt;
    } catch (error) {
        console.error("Error sending transaction: ", error);
        throw error;
    }
}

export const triggerAggregatorSelection = async () => {
    const abi = JSON.parse(fs.readFileSync("./abi/AggregatorSelection.json", "utf-8"));
    const address = aggregator_address;
    const contract = new web3.eth.Contract(abi, address);

    const account = web3.eth.accounts.privateKeyToAccount(privateKey);
    web3.eth.accounts.wallet.add(account);

    const gasPrice = await web3.eth.getGasPrice();
    const gasEstimate = await contract.methods.triggerAggregatorSelection().estimateGas({ from: account.address });

    const tx = {
        from: account.address,
        to: address,
        gas: gasEstimate,
        gasPrice: gasPrice,
        data: contract.methods.triggerAggregatorSelection().encodeABI(),
    };

    try {
        const signedTx = await web3.eth.accounts.signTransaction(tx, privateKey);
        const receipt = await web3.eth.sendSignedTransaction(signedTx.rawTransaction as string);
        console.log("Transaction receipt: ", receipt);
        return receipt;
    } catch (error) {
        console.error("Error sending transaction: ", error);
        throw error;
    }
}


   