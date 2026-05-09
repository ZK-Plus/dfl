// code adapted from pinata docs https://docs.pinata.cloud/quickstart/node-js
import 'dotenv/config';
import path from 'path';
import { fileURLToPath } from 'url';
import { getCurrentGM, setGlobalModel, getCurrentState, getAggregatorEndpoint, setAggregatorEndpoint, setCurrentState, setContribution, getTopContributor, triggerAggregatorSelection, getRound, incrementRound, isAuthorized, getDevicePublicKey, getPreviousAggregatorFromGMStorage, getLastRoundsAggregator } from "./bc_client.js";
import { getCurrentModel, pinFile, getFileFromIPFS, updateGM } from "./ipfs.js";
import fs from 'fs/promises';
import { DstackClient } from '@phala/dstack-sdk';
import crypto from 'crypto';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const deviceID = process.env.DEVICE_ID;
let currentState = "";
let aggregatorServerRunning = false;
const pythonServiceUrl = process.env.PYTHON_SERVICE_URL || 'http://127.0.0.1:8000';
const rsaPublicKey = process.env.RSA_PUBLIC_KEY.replace(/\\n/g, '\n') || "";
const rsaPrivateKey = process.env.RSA_PRIVATE_KEY.replace(/\\n/g, '\n') || "";

async function callPythonService(endpoint, payload = {}, { timeoutMs = 0 } = {}) {
    const controller = timeoutMs > 0 ? new AbortController() : null;
    const timeout = controller ? setTimeout(() => controller.abort(), timeoutMs) : null;
    try {
        const res = await fetch(`${pythonServiceUrl}${endpoint}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload),
            signal: controller?.signal,
        });
        const text = await res.text();
        let data = {};
        if (text) {
            try { data = JSON.parse(text); } catch { data = { raw: text }; }
        }
        if (!res.ok || data.ok === false) {
            throw new Error(`Python service ${endpoint} failed (${res.status}): ${text}`);
        }
        return data;
    } finally {
        if (timeout) clearTimeout(timeout);
    }
}

async function stopAggregatorServer() {
    if (!aggregatorServerRunning) return;
    await callPythonService('/server/stop');
    aggregatorServerRunning = false;
}

function derHexToBuffer(derHex) {
    if (typeof derHex !== 'string') return Buffer.alloc(0);
    let hex = derHex.trim();
    if (hex.startsWith('0x') || hex.startsWith('0X')) hex = hex.slice(2);
    hex = hex.replace(/\s+/g, '');
    if (hex.length === 0 || (hex.length % 2) !== 0) return Buffer.alloc(0);
    return Buffer.from(hex, 'hex');
}

async function verifyDownloadedGlobalModelSignature({ publicKeyDerHex, modelPath = './data/gm.bin', sigPath = './data/gm.bin.sig' } = {}) {
    const pubDer = derHexToBuffer(publicKeyDerHex);
    if (!pubDer.length) {
        console.error('Signature verification: invalid public key DER-hex');
        return false;
    }

    let modelBytes;
    let sigBytes;
    try {
        modelBytes = await fs.readFile(modelPath);
    } catch (e) {
        console.error(`Signature verification: cannot read model file ${modelPath}`, e);
        return false;
    }
    try {
        sigBytes = await fs.readFile(sigPath);
    } catch (e) {
        console.error(`Signature verification: cannot read signature file ${sigPath}`, e);
        return false;
    }
    if (!sigBytes || sigBytes.length === 0) {
        console.error('Signature verification: signature file empty');
        return false;
    }

    let keyObject;
    try {
        keyObject = crypto.createPublicKey({ key: pubDer, format: 'der', type: 'spki' });
    } catch (e) {
        console.error('Signature verification: failed to parse public key (DER/SPKI)', e);
        return false;
    }

    try {
        const ok = crypto.verify(
            'RSA-SHA256',
            modelBytes,
            { key: keyObject, padding: crypto.constants.RSA_PKCS1_PADDING },
            sigBytes
        );
        return !!ok;
    } catch (e) {
        console.error('Signature verification: crypto.verify failed', e);
        return false;
    }
}

const stateMachine = async () => {
    while (Number(await getRound()) < Number(process.env.ROUND)) {
        let state = await getCurrentState();

        switch (state["0"]) {
            case "TRAINING":
                if (state[1] === process.env.ACCOUNT_ADDRESS) {
                    console.log("I am the aggregator");
                    console.log("Round %d.", Number(await getRound()));
                    console.log("Starting the zerompq server ...");
                    try {
                        if (!aggregatorServerRunning) {
                            await callPythonService('/server/start', {
                                client_limit: Number(process.env.CLIENT_LIMIT || 1),
                            });
                            aggregatorServerRunning = true;
                        }
                        await setCurrentState("AGGREGATING");
                        continue;
                    } catch (e) {
                        console.error("Error during starting the aggregator server:", e);
                        return;
                    }
                } else {
                    console.log("I am not the aggregator");
                    console.log("Round %d.", Number(await getRound()));
                    console.log("Fetching the global model from IPFS ...");
                    await getCurrentModel();

                    const prevGM = await getCurrentGM();

                    const lastSignerAddress = await getLastRoundsAggregator();
                    const lastSignersPubKey = await getDevicePublicKey(lastSignerAddress);
                    const sigOk = await verifyDownloadedGlobalModelSignature({
                        publicKeyDerHex: lastSignersPubKey,
                        modelPath: "./data/gm.bin",
                        sigPath: "./data/gm.bin.sig",
                    });
                    if (!sigOk) {
                        console.error("Global model signature verification FAILED. Aborting training.");
                        return;
                    }
                    console.log("Global model signature verification successful.");

                    console.log("Starting local training ...");
                    try {
                        await callPythonService('/train', {
                            epochs: Number(process.env.EPOCH),
                            aggregator_public_key_der_hex: await getDevicePublicKey(state[1]),
                        });
                    } catch (e) {
                        console.error("Error during local training:", e);
                        return;
                    }
                    console.log("Local training complete.");

                    if (process.env.DOCKER === "phala") {
                        console.log("Fetching TDX Quote ...");
                        const client = new DstackClient();
                        const devClient = new DstackClient('http://localhost:8090');

                        // Get TEE instance information
                        const info = await client.info();
                        console.log('App ID:', info.app_id);
                        console.log('Instance ID:', info.instance_id);
                        console.log('App Name:', info.app_name);
                        console.log('TCB Info:', info.tcb_info);

                        // Generate remote attestation quote
                        const applicationData = JSON.stringify({
                            version: '1.0.0',
                            timestamp: Date.now(),
                            user_id: 'alice'
                        });

                        const quote = await client.getQuote(applicationData);
                        console.log('TDX Quote:', quote.quote);

                    }
                    console.log("Is the device authorized? ", await isAuthorized(process.env.ACCOUNT_ADDRESS));
                    console.log("Starting the zerompq client ...");
                    try {
                        await callPythonService('/client', {
                            server_ip: String(state["1"]),
                            device_id: String(deviceID),
                        });
                        await setContribution([process.env.ACCOUNT_ADDRESS]);
                    } catch (e) {
                        console.error("Error during model transfer:", e);
                        return;
                    }
                    console.log("Top contributor:", await getTopContributor());
                    console.log("Transfer complete. Send local model to aggregator.");
                    console.log("Rounds left: ", (Number(process.env.ROUND) - Number(await getRound())));
                    console.log("Waiting till next round.");
                    try {
                        const newGM = await waitForGMUpdate(prevGM, { pollMs: 5000, timeoutMs: 30 * 1000 });
                        console.log("New Global Model detected:", newGM);
                    } catch (e) {
                        console.warn("No new GM within timeout. Continuing loop.");
                    }
                    await sleep(2000);
                    continue;
                }

            case "AGGREGATING":
                if (state[1] === process.env.ACCOUNT_ADDRESS) {
                    currentState = "AGGREGATING";
                    console.log("I am the aggregator");
                    console.log("Starting the aggregation process ...");
                    try {
                        const expected = Number(process.env.CLIENT_LIMIT || 1);
                        const present = await waitForModels(expected, { dir: srcModelsDir, pollMs: 2000 });
                        console.log(`Models present before aggregation: ${present}/${expected}`);

                        if (aggregatorServerRunning) {
                            console.log("Stopping aggregator server before aggregation...");
                            await stopAggregatorServer();
                        }

                        const count = await stageAggregation();
                        console.log(`Staged ${count} model file(s) for aggregation.`);
                        if (count <= 0) {
                            console.log("No models to aggregate (count=0). Waiting for next loop.");
                            await sleep(2000);
                            continue;
                        }
                        if (count < expected) {
                            console.log(`Aggregating with ${count}/${expected} models.`);
                        }

                        await callPythonService('/aggregate', {
                            num_files: Number(count),
                        }, { timeoutMs: 3 * 60 * 1000 });
                    } catch (e) {
                        console.error("Error during aggregation:", e);
                        await sleep(2000);
                        continue;
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
                    console.log("Current Global Model:", await getCurrentGM());
                    await stageCleaning();
                    console.log("Passed cleaning");
                    await incrementRound();
                    console.log("Rounds left: ", (Number(process.env.ROUND) - Number(await getRound())));
                    await setCurrentState("TRAINING");
                    console.log("Set state to TRAINING for next round");
                    await triggerAggregatorSelection();
                    console.log("Triggered new aggregator selection");

                    continue;
                }
                //return;
                break;

            default:
                console.log("Unknown state:", state);
                currentState = "IDLE";
                await sleep(2000);
                continue; // nicht beenden
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

async function countBinFiles(dir) {
    try {
        const entries = await fs.readdir(dir, { withFileTypes: true });
        return entries.filter(e => e.isFile() && e.name.endsWith('.bin')).length;
    } catch (e) {
        if (e && e.code === 'ENOENT') return 0;
        throw e;
    }
}

async function waitForModels(expected, { dir, pollMs = 2000, timeoutMs = 10 * 60 * 1000 } = {}) {
    const start = Date.now();
    while (true) {
        const n = await countBinFiles(dir);
        if (n >= expected) {
            console.log(`Received ${n}/${expected} model files.`);
            return n; // Anzahl zurückgeben
        }
        if (Date.now() - start > timeoutMs) {
            console.warn(`Timeout waiting for ${expected} models. Proceeding with ${n} present in ${dir}.`);
            return n; // mit aktueller Anzahl fortfahren
        }
        await sleep(pollMs);
    }
}

async function waitForGMUpdate(prevCid, { pollMs = 3000, timeoutMs = 10 * 60 * 1000 } = {}) {
    const start = Date.now();
    while (true) {
        const cid = await getCurrentGM();
        if (cid && cid !== prevCid) return cid;
        if (Date.now() - start > timeoutMs) {
            throw new Error("Timeout waiting for aggregator signal (new Global Model).");
        }
        await sleep(pollMs);
    }
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
    stopAggregatorServer().catch(() => {});
    process.exit(0);
});
process.on('SIGTERM', () => {
    console.log('SIGTERM received. Exiting.');
    stopAggregatorServer().catch(() => {});
    process.exit(0);
});

async function stageCleaning() {
    await Promise.all([
        cleanFilesInDir(srcModelsDir),
        cleanFilesInDir(resultsIIDDir),
    ]);
    console.log("Cleaned files in %s and %s", srcModelsDir, resultsIIDDir);
}

async function cleanFilesInDir(dir) {
    try {
        const entries = await fs.readdir(dir, { withFileTypes: true });
        await Promise.all(entries.map(async (e) => {
            const full = path.join(dir, e.name);
            if (e.isFile() || e.isSymbolicLink()) {
                await fs.unlink(full).catch(() => {});
            }
        }));
    } catch (err) {
        if (err && err.code === 'ENOENT') return;
        throw err;
    }
}
