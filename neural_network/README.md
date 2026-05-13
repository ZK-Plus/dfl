# Thesis PyTorch Compatibility Adapter

This example ports the C++ interface to Python and
PyTorch while keeping the Node blockchain orchestrator contract unchanged.

The CLI intentionally mirrors the C++ executable:

```bash
python -m neural_network.cli get_random_wb
python -m neural_network.cli train 30 <aggregator-public-key-der-hex>
python -m neural_network.cli server <client-limit> [private-key.pem]
python -m neural_network.cli client <aggregator-ip> <device-id>
python -m neural_network.cli aggregate <num-files>
```

It reads and writes the original six-matrix binary layout:

```text
W1, B1, W2, B2, W3, B3 as little-endian float64 values in Eigen column-major order
```

The active model is the original MNIST MLP:

```text
784 -> 200 -> 50 -> 10
tanh -> tanh -> logits
```

The Node server talks to `neural_network/service.py` over
HTTP. The service exposes endpoints for training, transfer, aggregation, and
the ZMQ aggregation server lifecycle.

## Dependencies

Use the repository virtual environment and install:

```bash
pip install torch cryptography pyzmq
```
