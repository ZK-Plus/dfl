import axios from "axios";
import fs from "fs";
import FormData from "form-data";
import { setGlobalModelAndSignature, getCurrentGM, getCurrentGMSignature } from "./bc_client.js";
export const pinFile = async (filePath) => {
    try {
        console.log("Starting IPFS upload ...");
        const formData = new FormData();
        const file = fs.createReadStream(filePath);
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

            const base = (filePath.split("/").pop() || "file");
            const mfsPath = `/${new Date().toISOString().replace(/[:.]/g, "-")}-${base}`;
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
export const getFileFromIPFS = async (hash, outPath = "./data/gm.bin") => {
    try {
        if (process.env.IPFS_PROVIDER === "pinata") {
            console.log("Fetching from Pinata gateway ...");
            const res = await axios.get(process.env.IPFS_GATEWAY + `/ipfs/${hash}`);
            // write the file which is in binary format to the local file system
            fs.writeFileSync(outPath, res.data, { encoding: "binary" });
            console.log("File written to the local file system");
        } else {
            console.log("Fetching from Kubo ...");
            const url = `${process.env.KUBO_API}/api/v0/cat?arg=${encodeURIComponent(hash)}`;
            const res = await axios.post(url, null, { responseType: "arraybuffer" });
            // write the file which is in binary format to the local file system
            fs.writeFileSync(outPath, Buffer.from(res.data));
            console.log("File written to the local file system");
        }
        
    } catch (error) {
        console.log(error);
    }
};
export const updateGM = async () => {
    const modelPath = "./data/results_iid/aggregated.bin";
    const sigPath = "./data/results_iid/aggregated.bin.sig";
    const modelCid = await pinFile(modelPath);
    if (!modelCid)
        throw new Error("Pinning failed for model, no CID returned");
    const sigCid = await pinFile(sigPath);
    if (!sigCid)
        throw new Error("Pinning failed for signature, no CID returned");

    console.log("New GM CID:", modelCid);
    console.log("New GM SIG CID:", sigCid);

    await setGlobalModelAndSignature(modelCid, sigCid);
    console.log("Global model + signature updated (on-chain)");
};

export const getCurrentModel = async () => {
    const modelCid = await getCurrentGM();
    const sigCid = await getCurrentGMSignature();
    console.log("Model CID:", modelCid);
    console.log("Sig CID:", sigCid);
    if (typeof modelCid === 'string' && modelCid.length > 0) {
        await getFileFromIPFS(modelCid, "./data/gm.bin");
    }
    if (typeof sigCid === 'string' && sigCid.length > 0) {
        await getFileFromIPFS(sigCid, "./data/gm.bin.sig");
    }
    console.log("Global model" + (sigCid ? " + signature fetched" : ""));
};
