FROM node:20-slim

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONUNBUFFERED=1
ENV PIP_NO_CACHE_DIR=1
ENV THESIS_NODE_SERVER_DIR=/dfl/node_server
ENV PYTHON_SERVICE_HOST=0.0.0.0
ENV PYTHON_SERVICE_PORT=8000

WORKDIR /dfl

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        bash \
        curl \
        python3 \
        python3-venv \
        python3-pip && \
    rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/venv && \
    /opt/venv/bin/python -m pip install --upgrade pip && \
    /opt/venv/bin/python -m pip install cryptography numpy pyzmq && \
    /opt/venv/bin/python -m pip install --index-url https://download.pytorch.org/whl/cpu torch

ENV PATH="/opt/venv/bin:${PATH}"

COPY data/mnist ./config
COPY data/rsa_keys ./keys

COPY dfl/node_server/package*.json ./node_server/
WORKDIR /dfl/node_server
RUN npm ci

WORKDIR /dfl
COPY dfl/node_server ./node_server
COPY dfl/neural_network ./neural_network
COPY dfl/start_node_neural_network.sh ./start_node_neural_network.sh

COPY data/mnist/data/train-images.idx3-ubyte ./node_server/data/train-images.idx3-ubyte
COPY data/mnist/data/train-labels.idx1-ubyte ./node_server/data/train-labels.idx1-ubyte
COPY data/mnist/data/t10k-images.idx3-ubyte ./node_server/data/t10k-images.idx3-ubyte
COPY data/mnist/data/t10k-labels.idx1-ubyte ./node_server/data/t10k-labels.idx1-ubyte
COPY data/mnist/data/random_start.bin ./node_server/data/random_start.bin
COPY data/phala_tdx_quote ./node_server/attestation/

RUN chmod +x /dfl/start_node_neural_network.sh && \
    mkdir -p /dfl/node_server/data/results_iid /dfl/node_server/received_models /dfl/node_server/attestation

WORKDIR /dfl/node_server

EXPOSE 8000 5555

CMD ["node", "/dfl/node_server/dist/server.js"]
