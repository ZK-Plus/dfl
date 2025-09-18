import axios from "axios";
import fs from "fs";
import FormData from "form-data";
import { setGlobalModel, getCurrentGM } from "./bc_client.js";
export const pinFile = async () => {
    try {
        console.log("Starting IPFS upload ...");
        const formData = new FormData();
        //const file = fs.createReadStream("./data/backup.bin");
        //const file = fs.createReadStream("./data/test.txt");
        const file = fs.createReadStream("./data/results_iid/aggregated.bin");
        formData.append("file", file);

        if (process.env.IPFS_PROVIDER === "pinata") {
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
            console.log("IPFS Uploaded.");
            return res.data.IpfsHash;
        } else {
            const url = `${process.env.KUBO_API}/api/v0/add?pin=true&cid-version=1&wrap-with-directory=false`;
            const res = await axios.post(url, formData, {
                headers: formData.getHeaders(),
                maxBodyLength: Infinity,
                maxContentLength: Infinity,
            });
            const cid = res.data?.Hash;
            if (!cid) throw new Error(`Kubo add returned no CID: ${JSON.stringify(res.data)}`);

            const mfsPath = `/${new Date().toISOString().replace(/[:.]/g, "-")}-aggregated.bin`;
            const cpUrl = `${process.env.KUBO_API}/api/v0/files/cp?arg=${encodeURIComponent(`/ipfs/${cid}`)}&arg=${encodeURIComponent(mfsPath)}&parents=true`;
            await axios.post(cpUrl);
            console.log(`Copied ${cid} to MFS at ${mfsPath}`);

            const pinUrl = `${process.env.KUBO_API}/api/v0/pin/ls?arg=${cid}`;
            const pinRes = await axios.post(pinUrl);
            console.log("Pin status:", pinRes.data?.Keys ? "pinned" : "unknown");
            
            return cid;
        }

    }
    catch (error) {
        console.log(error);
    }
};
export const getFileFromIPFS = async (hash) => {
    try {
        if (process.env.IPFS_PROVIDER === "pinata") {
            console.log("Fetching from Pinata gateway ...");
            const res = await axios.get(process.env.IPFS_GATEWAY + `/ipfs/${hash}`);
            // write the file which is in binary format to the local file system
            fs.writeFileSync(`./data/gm.bin`, res.data, { encoding: "binary" });
            console.log("File written to the local file system");
        } else {
            console.log("Fetching from Kubo ...");
            const url = `${process.env.KUBO_API}/api/v0/cat?arg=${encodeURIComponent(hash)}`;
            const res = await axios.post(url, null, { responseType: "arraybuffer" });
            // write the file which is in binary format to the local file system
            fs.writeFileSync(`./data/gm.bin`, Buffer.from(res.data));
            console.log("File written to the local file system");
        }
        
    } catch (error) {
        console.log(error);
    }
};
export const updateGM = async () => {
    const ipfsAddress = await pinFile();
    if (!ipfsAddress) throw new Error("Pinning failed, no CID returned");
    console.log("New GM CID:", ipfsAddress);
    await setGlobalModel(ipfsAddress); // wichtig: warten, damit Nonce sauber hochzählt
    console.log("Global model updated");
};
export const getCurrentModel = async () => {
    const ipfsAddress = await getCurrentGM();
    console.log(ipfsAddress);
    if (typeof ipfsAddress === 'string') {
        await getFileFromIPFS(ipfsAddress);
        console.log("Global model retrieved");
    }
};
