#include "train-network2.h"
#include <thread>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "zmq.hpp"

namespace fs = std::filesystem;

void start_zerompq_server()
{
    zmq::context_t context{1};
    zmq::socket_t socket{context, zmq::socket_type::rep};
    socket.bind("tcp://*:5555");
    std::cout << "Server started\n";

    fs::create_directory("./received_models"); // Ensure the directory exists

    while (true)
    {
        zmq::message_t request;

        // Receive the filename (not used in this simplified example)
        socket.recv(request);
        std::string filename = request.to_string();
        std::cout << "Receiving file: " << filename << std::endl;

        // Acknowledge filename receipt
        socket.send(zmq::str_buffer("Filename OK"), zmq::send_flags::none);

        // Receive the file contents
        socket.recv(request);
        std::string fileContents = request.to_string();

        // Save the file
        std::ofstream outFile("./received_models/" + filename, std::ios::binary);
        outFile << fileContents;
        outFile.close();

        // Acknowledge file receipt
        socket.send(zmq::str_buffer("File received"), zmq::send_flags::none);
    }
}

void start_zerompq_client(const std::string serverIP)
{
    zmq::context_t context{1};
    zmq::socket_t socket{context, zmq::socket_type::req};
    socket.connect("tcp://" + serverIP + ":5555");

    // Prepare the filename (not actually used to name the file in this example)
    std::string filename = "simple_rq_socket.bin";

    // Send the filename
    socket.send(zmq::buffer(filename), zmq::send_flags::none);

    // Wait for filename ack
    zmq::message_t reply;
    socket.recv(reply);
    std::cout << "Server: " << reply.to_string() << std::endl;

    // Read the file content
    std::ifstream file("./src/double hidden layer/w_and_b.bin", std::ios::binary);
    std::vector<char> contents((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

    // Send the file content
    socket.send(zmq::buffer(contents), zmq::send_flags::none);

    // Wait for file receipt confirmation
    socket.recv(reply);
    std::cout << "Server: " << reply.to_string() << std::endl;
}

// main function with command line arguments to train the network
int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        if (std::string(argv[1]) == "server")
        {
            start_zerompq_server();
            return 0;
        }
        else if (std::string(argv[1]) == "client")
        {
            start_zerompq_client(argv[2]);
            return 0;
        }
        else if (std::string(argv[1]) == "train")
        {
            train_network();
            return 0;
        }
        else
        {
            std::cout << "Invalid argument\n";
        }
    }

    {
        std::cout << "Please provide an argument to train the network\n";
    }

    return 0;
}
