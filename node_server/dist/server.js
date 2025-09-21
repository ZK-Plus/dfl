// code adapted from pinata docs https://docs.pinata.cloud/quickstart/node-js
import 'dotenv/config';
import path from 'path';
import { fileURLToPath } from 'url';
import child_process from 'child_process';
import util from 'util';
import { getCurrentGM, setGlobalModel, getCurrentState, getAggregatorEndpoint, setAggregatorEndpoint, setCurrentState, setContribution, getTopContributor, triggerAggregatorSelection} from "./bc_client.js";
import { getCurrentModel, pinFile, getFileFromIPFS, updateGM } from "./ipfs.js";
import fs from 'fs/promises';

const trainingProcess = util.promisify(child_process.execFile);
// Define __dirname manually
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const exePath = path.join(__dirname, '../start.exe');
const deviceID = process.env.DEVICE_ID;
let currentState = "";
let aggregatorProc = null;
const stateMachine = async () => {
    while (true) {
        let state = await getCurrentState();

        switch (state["0"]) {
            case "TRAINING":
                if (state[1] === process.env.ACCOUNT_ADDRESS) {
                    console.log("I am the aggregator");
                    console.log("Starting the zerompq server ...");
                    try {
                        const { stdout, stderr } = await trainingProcess(exePath, ["server"]);
                        console.log('stdout:', stdout);
                        if (stderr) console.error('stderr:', stderr);
                        await setCurrentState("AGGREGATING");
                        continue;
                    } catch (e) {
                        console.error("Error during starting the aggregator server:", e);
                        return;
                    }
                } else {
                    console.log("I am not the aggregator");
                    console.log("Fetching the global model from IPFS ...");
                    await getCurrentModel();
                    console.log("Starting local training ...");
                    try {
                        const { stdout, stderr } = await trainingProcess(exePath, ["train", String("10")]);
                        console.log('stdout:', stdout);
                        if (stderr) {
                            console.error('stderr:', stderr);
                        }
                    } catch (e) {
                        console.error("Error during local training:", e);
                        return;
                    }
                    console.log("Local training complete.");
                    // transfer the local model to the aggregator by calling another executable
                    console.log("Starting the zerompq client ...");
                    try {
                        const { stdout, stderr } = await trainingProcess(exePath, ["client", String(state["1"]), String(deviceID)]);
                        console.log('stdout:', stdout);
                        if (stderr) {
                            console.error('stderr:', stderr);
                        }
                        await setContribution([process.env.ACCOUNT_ADDRESS]);
                    } catch (e) {
                        console.error("Error during model transfer:", e);
                        return;
                    }
                    console.log("Transfer complete. Send local model to aggregator.");
                    console.log("Shutting down client.");
                    process.exit(0);          
                }
                break;

            case "AGGREGATING":
                if (state[1] === process.env.ACCOUNT_ADDRESS) {
                    currentState = "AGGREGATING";
                    console.log("I am the aggregator");
                    console.log("Starting the aggregation process ...");
                    try {
                        const count = await stageAggregation();
                        const { stdout, stderr } = await trainingProcess(exePath, ["aggregate", String(count)]);
                        console.log('stdout:', stdout);
                        if (stderr) {
                            console.error('stderr:', stderr);
                        }
                    } catch (e) {
                        console.error("Error during aggregation:", e);
                        return;
                    }
                    console.log("Aggregation complete.");
                    await setCurrentState("UPDATING");
                    continue;
                }
                break;

            case "UPDATING":
                if (state[1] === process.env.ACCOUNT_ADDRESS) {
                    currentState = "UPDATING";
                    console.log("I am the aggregator");
                    console.log("Starting the updating process ...");
                    try {
                        await updateGM();
                    } catch (e) {
                        console.error("Error during updating the global model:", e);
                        return;
                    }
                    console.log("Updating complete.");
                    //await setCurrentState("IDLE");
                    console.log("Shutting down aggregator.");
                    process.exit(0);
                }
                return;

            default:
                currentState = "IDLE";
                return;
        }
    }
};

const srcModelsDir = path.join(__dirname, '../received_models');
const resultsIIDDir = path.join(__dirname, '../data/results_iid');
async function stageAggregation() {
    await fs.mkdir(resultsIIDDir, { recursive: true });

    try {
        const destEntries = await fs.readdir(resultsIIDDir);
        await Promise.all(
            destEntries
                .filter(n => n.endsWith('.bin'))
                .map(n => fs.unlink(path.join(resultsIIDDir, n)).catch(() => {}))
        );
    } catch {}

    // Quelle lesen und .bin-Dateien sortiert verarbeiten
    const entries = await fs.readdir(srcModelsDir, { withFileTypes: true });
    const binFiles = entries
        .filter(e => e.isFile() && e.name.endsWith('.bin'))
        .map(e => e.name)
        .sort((a, b) => a.localeCompare(b));

    if (binFiles.length === 0) {
        console.log("No .bin files found in %s", srcModelsDir);
        return 0;
    }

    console.log("Copied files (with rename):");
    let idx = 0;
    for (const name of binFiles) {
        const src = path.join(srcModelsDir, name);
        const destName = `wb_client_${idx}.bin`;
        const dest = path.join(resultsIIDDir, destName);
        await fs.copyFile(src, dest);
        console.log(" - %s -> %s", src, dest);
        idx++;
    }

    return binFiles.length;
}

async function sleep(ms) {
    return new Promise(r => setTimeout(r, ms));
}

async function runService() {
    try {
        await stateMachine();
    } catch (e) {
        console.error('stateMachine error:', e);
    }
}

runService();


process.on('SIGINT', () => {
    console.log('SIGINT received. Exiting.');
    process.exit(0);
});
process.on('SIGTERM', () => {
    console.log('SIGTERM received. Exiting.');
    process.exit(0);
});
