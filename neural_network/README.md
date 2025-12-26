# Neural Network for Federated Learning

This repository contains the backbone for a federated learning system. It builds upon the [Digit-Recognition](https://github.com/Mimsqueeze/Digit-Recognition) repository for digit recognition but has been highly extended with the logic for federated learning and communication using the ZeroMQ message broker.

## Structure

- **src/**: Contains the main source code for the neural network and federated learning logic.
- **test/**: Contains test scripts written in Python to validate the functionality of the system.

## Features

- **Federated Learning**: Implements federated learning to train models across multiple decentralized devices without sharing raw data.
- **ZeroMQ Communication**: Utilizes ZeroMQ as a message broker for efficient communication between nodes.

This project builds on the [Digit-Recognition](https://github.com/Mimsqueeze/Digit-Recognition) repository. Special thanks to the original authors for their work.

## Neural Network calls for local use

Change into the neural_network folder.

To simulate 3 rounds of training the federated averag.

```
./start.exe simulate
```
To train one round.
```
./start.exe train 30 
```
To start the zerompq server.
```
./start.exe server
```
To start the clients.
```
./start.exe client * 1
```
To calculate the federated average.
```
./start.exe aggregate 3
```
