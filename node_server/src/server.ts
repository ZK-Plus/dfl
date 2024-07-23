// code adapted from pinata docs https://docs.pinata.cloud/quickstart/node-js

import 'dotenv/config';
import path from 'path';
import { fileURLToPath } from 'url';
import child_process from 'child_process';
import { getCurrentGM, setGlobalModel, getCurrentState } from "./bc_client.js";
import { getCurrentModel, pinFile, getFileFromIPFS } from "./ipfs.js";

const trainingProcess = child_process.execFile;

// Define __dirname manually
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

let currentState = "";

const stateMachine = async () => {
  let state = await getCurrentState();
  console.log(state);

  switch ("TRAINING" as string) {
    case "TRAINING":
      // check if this node is the current aggregator
      if ((state as any)[1] === process.env.ACCOUNT_ADDRESS) {
        console.log("I am the aggregator");
        //! open socket
        const exePath = path.join(__dirname, '../start.exe');
        trainingProcess(exePath, ["server"] , function (err: any, data: any) {
          if (err) {
            console.error("Error executing training process:", err);
            return;
          }
          console.log(data.toString());
        });
      } else {
        console.log("I am not the aggregator");
        //! start training process
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

stateMachine();
