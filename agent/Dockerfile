FROM python:3.11-slim

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONUNBUFFERED=1
ENV PIP_NO_CACHE_DIR=1
ENV PYTHONPATH=/app

WORKDIR /app

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        curl && \
    rm -rf /var/lib/apt/lists/*

COPY agent ./agent
COPY data/mnist ./data/mnist

RUN python -m pip install --upgrade pip setuptools wheel && \
    python -m pip install -r agent/requirements.txt

CMD ["python", "agent/run_agent.py", "--source", "contract", "--rpc-url", "http://127.0.0.1:8545", "--gm-storage-address", "0x9fE46736679d2D9a65F0992F2272dE9f3c7fa6e0", "--registry-address", "0x5FbDB2315678afecb367f032d93F642f64180aa3", "--ipfs-api-url", "http://127.0.0.1:5001", "--skip-calibration"]
