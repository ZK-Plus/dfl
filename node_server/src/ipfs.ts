import axios from "axios";
import fs from "fs";
import FormData from "form-data";

import { setGlobalModel, getCurrentGM } from "./bc_client.js";

export const pinFile = async () => {
  
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
  
      const res = await axios.post(
        "https://api.pinata.cloud/pinning/pinFileToIPFS",
        formData,
        {
          headers: {
            Authorization: `Bearer ${process.env.PINATA_JWT}`,
          },
        }
      );
      return res.data.IpfsHash;
    } catch (error) {
      console.log(error);
    }
  }
  
  export const getFileFromIPFS = async (hash: string) => {
    try {
      const res = await axios.get(`https://turquoise-zestful-squirrel-651.mypinata.cloud/ipfs/${hash}`);
      // write the file which is in binary format to the local file system
      fs.writeFileSync
        (`./data/gm.bin`, res.data, { encoding: "binary" });
      console.log("File written to the local file system");
    } catch (error) {
      console.log(error);
    }
  }
  
  export const updateGM = async () => {
    const ipfsAddress = await pinFile();
    console.log(ipfsAddress);
    setGlobalModel(ipfsAddress);
    console.log("Global model updated");
  }
  
  export const getCurrentModel = async () => {
    const ipfsAddress = await getCurrentGM();
    console.log(ipfsAddress);
    if (typeof ipfsAddress === 'string') {
      await getFileFromIPFS(ipfsAddress);
      console.log("Global model retrieved");
    }
  }