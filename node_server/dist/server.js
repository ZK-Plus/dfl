// code adapted from pinata docs https://docs.pinata.cloud/quickstart/node-js
import axios from "axios";
import fs from "fs";
import FormData from "form-data";
import 'dotenv/config';
import { getCurrentGM, setGlobalModel } from "./bc_client.js";
const pinFile = async () => {
    try {
        const formData = new FormData();
        const file = fs.createReadStream("./data/backup.bin");
        formData.append("file", file);
        const pinataMetadata = JSON.stringify({
            name: "File name",
        });
        formData.append("pinataMetadata", pinataMetadata);
        const pinataOptions = JSON.stringify({
            cidVersion: 1,
        });
        formData.append("pinataOptions", pinataOptions);
        const res = await axios.post("https://api.pinata.cloud/pinning/pinFileToIPFS", formData, {
            headers: {
                Authorization: `Bearer ${process.env.PINATA_JWT}`,
            },
        });
        return res.data.IpfsHash;
    }
    catch (error) {
        console.log(error);
    }
};
const getFileFromIPFS = async (hash) => {
    try {
        const res = await axios.get(`https://turquoise-zestful-squirrel-651.mypinata.cloud/ipfs/${hash}`);
        // write the file which is in binary format to the local file system
        fs.writeFileSync(`./data/gm.bin`, res.data, { encoding: "binary" });
        console.log("File written to the local file system");
    }
    catch (error) {
        console.log(error);
    }
};
const updateGM = async () => {
    const ipfsAddress = await pinFile();
    console.log(ipfsAddress);
    setGlobalModel(ipfsAddress);
    console.log("Global model updated");
};
const getCurrentModel = async () => {
    const ipfsAddress = await getCurrentGM();
    console.log(ipfsAddress);
    if (typeof ipfsAddress === 'string') {
        await getFileFromIPFS(ipfsAddress);
        console.log("Global model retrieved");
    }
};
//pinFile();
//getFileFromIPFS("bafybeifoxfj4rtcd6ctag62haaxnpozkf66cyyxsuys5ffhmuovhxq4clm");
//getBalance();
//getCurrentGM().then(res => console.log(res));
// setGlobalModel("hello from local bc client");
getCurrentModel();
//updateGM();
