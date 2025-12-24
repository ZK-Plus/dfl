# Proof of concept implementation: Practical Verifiable Decentralized Federated Learning with TEEs

### Advancing the Efficiency of Decentralized Federated Learning

The folders represent different components of the MVP.

- Docker setup: Recommended.
- Neural Network: ANN with additional FedAvg algorithm.
- Node Server: Logic for the components inside the edge devices, encapsulates the Nural Network.
- Remote attestation Code: zkVM-based remote attestation and Foundry Project including all smart contracts and interfaces.

![Decentralized Federated Learning](./node_server/arch/operational_flow.jpg "Operational Flow")


## Docker (Recommended) 

We recommend you use the docker files. All other guides (non docker) are for leaning and documentation purpous.

See [Docker README](./docker/README.md).

## Other Readme´s
- [Neural Network README](./neural_network/README.md)
- [Node Server README](./node_server/README.md)
- [Remote Attestation README](./remote_attestation/README.md)

## Importent configuration files

- The [.env file](.docker/README.md) you have to create yourself has the round configuration (i.e. epoch, round)
- The [starter_docker file](.remote_attestation/starter_docker.sh) is a script that deployes the smart contracts and sets the initial values (i.e. register workers etc.)
