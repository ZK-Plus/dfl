#!/usr/bin/env bash
set -euo pipefail

cleanup() {
    if [ -n "${PYTHON_PID:-}" ]; then
        kill "$PYTHON_PID" 2>/dev/null || true
    fi
}
trap cleanup INT TERM EXIT

cp "${TRAIN_IMAGES_SRC}" /dfl/node_server/data/train-images.idx3-ubyte
cp "${TRAIN_LABELS_SRC}" /dfl/node_server/data/train-labels.idx1-ubyte
cp /dfl/config/test_data/t10k-images.idx3-ubyte /dfl/node_server/data/t10k-images.idx3-ubyte
cp /dfl/config/test_data/t10k-labels.idx1-ubyte /dfl/node_server/data/t10k-labels.idx1-ubyte
if [ -n "${RSA_PRIVATE_KEY:-}" ] || [ -n "${RSA_PUBLIC_KEY:-}" ]; then
    python - <<'PY'
import os
from pathlib import Path

for env_name, out_path in (
    ("RSA_PRIVATE_KEY", "/dfl/node_server/private_key.pem"),
    ("RSA_PUBLIC_KEY", "/dfl/node_server/public_key.pem"),
):
    value = os.environ.get(env_name)
    if value:
        Path(out_path).write_text(value.replace("\\n", "\n"), encoding="utf-8")
PY
else
    cp "${RSA_PRIVATE_KEY_FILE}" /dfl/node_server/private_key.pem
    cp "${RSA_PUBLIC_KEY_FILE}" /dfl/node_server/public_key.pem
fi

python /dfl/python_worker/start_service.py 2> >(grep -v "Could not initialize NNPACK" >&2) &
PYTHON_PID=$!

python - <<'PY'
import time
import urllib.request

deadline = time.time() + 60
while time.time() < deadline:
    try:
        urllib.request.urlopen("http://127.0.0.1:8000/health", timeout=2).read()
        raise SystemExit(0)
    except Exception:
        time.sleep(1)
raise SystemExit("Python ML service did not become healthy")
PY

exec node /dfl/node_server/dist/server.js
