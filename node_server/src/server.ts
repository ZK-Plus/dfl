// code adapted from pinata docs https://docs.pinata.cloud/quickstart/node-js

import 'dotenv/config';
import path from 'path';
import { fileURLToPath } from 'url';
import child_process from 'child_process';
import { getCurrentGM, setGlobalModel, getCurrentState, getAggregatorEndpoint, setAggregatorEndpoint, setCurrentState, setContribution, getTopContributor, triggerAggregatorSelection} from "./bc_client.js";
import { getCurrentModel, pinFile, getFileFromIPFS } from "./ipfs.js";

const trainingProcess = child_process.execFile;

// Define __dirname manually
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const exePath = path.join(__dirname, '../start.exe');

const deviceID = process.env.DEVICE_ID;

let currentState = "";

const stateMachine = async () => {

  let state = await getCurrentState();
  console.log(state);

  switch (state!["0"] as string) {
    case "TRAINING":
      // check if this node is the current aggregator
      if ((state as any)[1] === process.env.ACCOUNT_ADDRESS) {
        console.log("I am the aggregator");
        // open socket
        trainingProcess(exePath, ["server"] , function (err: any, data: any) {
          if (err) {
            console.error("Error executing training process:", err);
            return;
          }
          console.log(data.toString());
        });
      } else {
        console.log("I am not the aggregator");
        //fetch global model from IPFS
        await getCurrentModel();

        // execute the local training process
        trainingProcess(exePath, ["train"] , function (err: any, data: any) {
          if (err) {
            console.error("Error executing training process:", err);
            return;
          }
          console.log(data.toString());
        });

        // transfer the local model to the aggregator by calling another executable
        trainingProcess(exePath, ["transfer", String(state!["2"]), String(deviceID) ] , function (err: any, data: any) {
          if (err) {
            console.error("Error executing training process:", err);
            return;
          }
          console.log(data.toString());
        });
        

      }
      break;
    case "AGGREGATING":
      currentState = "AGGREGATING";
      break;
    case "UPDATING":
      currentState = "UPDATING";
      break;
    default:
      currentState = "IDLE";
  }
}

//setCurrentState("sent_by_node");
//setGlobalModel("test");

//const deviceAddress = "0x927507b066B40C5a6b0fd327eFE1c9d33edeE759";
const deviceAddress = "0x6004D395731f85938177d5B07aCfEc2D5D7ECcB0";
let callArray = [deviceAddress, deviceAddress, deviceAddress, deviceAddress,deviceAddress, deviceAddress];
let newAddresses = ["0xAB28fB54fA7488fAeb2De0d54F3eB660fE7AE983", "0x967d3cf74E88b993524FC477280a7B4Cf78f598f", "0x686F34C3cDF0d2b9e698d238484F986762e4F47a", "0x93D3Fe54d3e0Ec0DD6e79A96F0E437C98F2503F0", "0x1048e97a481F5245574be144b55a664aeA4d9707", "0x697eb5046d16f5CaA53A081C7Bec13A57f8574c3", "0x0E61A7DD4E52d85C50B874f8d7eA867394C9FC69", "0x9C8B63a2ea87DeDee7CACEe5F16f84806f1D9818", "0x84D029e64a318986eFA9F4Ce90583aEa225495b5", "0x1ED86F424c699bB460F92eeb8e125fd38ac05523", "0x4FfBAad1176f28eBDb61BB8996603426257dcc33", "0x15A29130c799254defBB08Bf551793089De84E45", "0x1d48CaA9cA9E8CBbe1150854Bf1486d7EA0db5B8", "0x13f2E970B47D54651BDa93138cFF65927bb26c45", "0x26B7Dfb8BCCCCe6e3E4b609676fEC1aF4f84e584", "0x46C1e55aec195b742885060B47F9e9766fafE1Ed"]

// for(let i = 6; i <= 15; i++) {
//   console.log('Round: ' + i);
//   await setContribution(callArray);
//   callArray.push(deviceAddress);
// }
//console.log(newAddresses.length);
for (let i = 0; i < 15; i++) {
  console.log('Round: ' + i);
  await setContribution(newAddresses.slice(0, i + 1));
  //console.log(newAddresses.slice(0, i + 1).length);

}


//setContribution(["0x927507b066B40C5a6b0fd327eFE1c9d33edeE759", "0x927507b066B40C5a6b0fd327eFE1c9d33edeE759", "0x927507b066B40C5a6b0fd327eFE1c9d33edeE759", "0x927507b066B40C5a6b0fd327eFE1c9d33edeE759", "0x927507b066B40C5a6b0fd327eFE1c9d33edeE759"]);

// getTopContributor().then((result) => {
//   console.log(result);
// });

// triggerAggregatorSelection().then((result) => {
//   console.log(result);
// });

// getCurrentState().then((result) => {
//   console.log(result);
// });