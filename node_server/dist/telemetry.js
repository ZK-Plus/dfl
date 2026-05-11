import crypto from 'crypto';

const endpoint = process.env.OTEL_EXPORTER_OTLP_TRACES_ENDPOINT || "";
let warned = false;

function nowUnixNano() {
    return String(BigInt(Date.now()) * 1000000n);
}

function traceIdForRound(round) {
    return crypto
        .createHash('sha256')
        .update(`dfl-round:${round}`)
        .digest('hex')
        .slice(0, 32);
}

function spanId() {
    return crypto.randomBytes(8).toString('hex');
}

function attributeValue(value) {
    if (typeof value === 'number' && Number.isFinite(value)) {
        return Number.isInteger(value)
            ? { intValue: String(value) }
            : { doubleValue: value };
    }
    if (typeof value === 'boolean') return { boolValue: value };
    return { stringValue: String(value ?? "") };
}

function attributes(values) {
    return Object.entries(values)
        .filter(([, value]) => value !== undefined && value !== null)
        .map(([key, value]) => ({
            key,
            value: attributeValue(value),
        }));
}

async function exportSpan(span, resourceAttributes = {}) {
    if (!endpoint) return;

    const payload = {
        resourceSpans: [
            {
                resource: {
                    attributes: attributes({
                        "service.name": process.env.OTEL_SERVICE_NAME || "dfl-node",
                        "deployment.environment": process.env.DOCKER || "local",
                        "dfl.account": process.env.ACCOUNT_ADDRESS || "",
                        "dfl.device_id": process.env.DEVICE_ID || "",
                        ...resourceAttributes,
                    }),
                },
                scopeSpans: [
                    {
                        scope: {
                            name: "dfl-node-state-machine",
                            version: "1.0.0",
                        },
                        spans: [span],
                    },
                ],
            },
        ],
    };

    try {
        const controller = new AbortController();
        const timeout = setTimeout(() => controller.abort(), 1000);
        const response = await fetch(endpoint, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload),
            signal: controller.signal,
        });
        clearTimeout(timeout);
        if (!response.ok && !warned) {
            warned = true;
            console.warn(`OTel trace export failed (${response.status})`);
        }
    } catch (error) {
        if (!warned) {
            warned = true;
            console.warn("OTel trace export failed:", error);
        }
    }
}

export async function recordRoundEvent(name, { round = 0, role = "", attributes: eventAttributes = {} } = {}) {
    const timestamp = nowUnixNano();
    await exportSpan({
        traceId: traceIdForRound(round),
        spanId: spanId(),
        name,
        kind: 1,
        startTimeUnixNano: timestamp,
        endTimeUnixNano: timestamp,
        attributes: attributes({
            "dfl.round": round,
            "dfl.role": role,
            ...eventAttributes,
        }),
    });
}

export async function recordRoundSpan(name, {
    round = 0,
    role = "",
    startTimeMs,
    endTimeMs,
    status = "OK",
    attributes: spanAttributes = {},
} = {}) {
    const start = Number.isFinite(startTimeMs) ? startTimeMs : Date.now();
    const end = Number.isFinite(endTimeMs) ? endTimeMs : Date.now();
    await exportSpan({
        traceId: traceIdForRound(round),
        spanId: spanId(),
        name,
        kind: 1,
        startTimeUnixNano: String(BigInt(Math.floor(start)) * 1000000n),
        endTimeUnixNano: String(BigInt(Math.floor(Math.max(end, start))) * 1000000n),
        attributes: attributes({
            "dfl.round": round,
            "dfl.role": role,
            "dfl.duration_ms": Math.max(0, Math.floor(end - start)),
            ...spanAttributes,
        }),
        status: {
            code: status === "ERROR" ? 2 : 1,
        },
    });
}
