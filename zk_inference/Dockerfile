FROM python:3.11-slim

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONUNBUFFERED=1
ENV PIP_NO_CACHE_DIR=1
ENV PYTHONPATH=/app:/app/zk_inference

WORKDIR /app

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        curl \
        git \
        libgomp1 && \
    rm -rf /var/lib/apt/lists/*

COPY zk_inference ./zk_inference
COPY dfl/neural_network ./dfl/neural_network
COPY data ./data

RUN python -m pip install --upgrade pip setuptools wheel && \
    python -m pip install --index-url https://download.pytorch.org/whl/cpu torch && \
    python -m pip install -r zk_inference/requirements.txt && \
    python -m pip install -e dfl/neural_network && \
    python -m pip install ezkl

EXPOSE 8090

CMD ["python", "zk_inference/server.py"]
