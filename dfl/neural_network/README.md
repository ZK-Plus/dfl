# Neural Network

This directory contains the active Python/PyTorch implementation used by the worker nodes.

It provides:

- the MNIST model definition,
- binary model serialization compatible with the original six-matrix layout,
- local training,
- encrypted model transfer helpers,
- federated averaging,
- global model signing,
- and an HTTP service used by `node_server`.

The old compatibility package has been flattened. The active imports are now directly under `neural_network`, for example:

```python
from neural_network.cli import FederatedMLP
from neural_network.service import main
```

By default, local training and inference helpers resolve MNIST files from `data/mnist/data` at the repository root.

## Model Layout

The model is the MNIST MLP used by the DFL prototype:

```text
784 -> 200 -> 50 -> 10
tanh -> tanh -> logits
```

The serialized model layout is:

```text
W1, B1, W2, B2, W3, B3
```

Values are stored as little-endian `float64` in Eigen column-major order.

## CLI

The CLI mirrors the original worker executable interface expected by the Node.js orchestration layer:

```bash
python -m neural_network.cli get_random_wb
python -m neural_network.cli train 30 <aggregator-public-key-der-hex>
python -m neural_network.cli server <client-limit> [private-key.pem]
python -m neural_network.cli client <aggregator-ip> <device-id>
python -m neural_network.cli aggregate <num-files>
```

## HTTP Service

The Docker worker starts the Python service and the Node.js worker process together. The service listens on port `8000` inside the container and exposes endpoints for:

- local training,
- client transfer,
- aggregation,
- starting and stopping the ZMQ aggregation server.

For local development:

```bash
python -m neural_network.service
```

## Install

From the repository root:

```bash
pip install -e dfl/neural_network
```

The core runtime dependencies are PyTorch, `cryptography`, and `pyzmq`.
