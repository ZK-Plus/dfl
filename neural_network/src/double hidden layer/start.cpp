#include "train-network2.h"
#include <thread>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "../functions.h"
#include "zmq.hpp"
#include <vector>
#include <Eigen/Dense>
#include "network2.h"

namespace fs = std::filesystem;
using Eigen::MatrixXd;

#include <iostream>

// ANSI escape codes for text color
#define GREEN_TEXT "\033[1;32m"
#define RESET_TEXT "\033[0m"

void printBoxLine(int length)
{
    std::cout << "+";
    for (int i = 0; i < length; i++)
    {
        std::cout << "-";
    }
    std::cout << "+\n";
}

void printBoxRow(const std::string &content, int length)
{
    std::cout << "| " << GREEN_TEXT << content << RESET_TEXT;
    for (int i = content.length() + 2; i < length; i++)
    {
        std::cout << " ";
    }
    std::cout << " |\n";
}

void runTest()
{
    std::string custom_weights_and_biases_file_path = "./data/results_iid/aggregated.bin";
    // string custom_weights_and_biases_file_path = WEIGHTS_AND_BIASES_FILE_PATH;
    //   Obtain the testing set
    MatrixXd X = get_images(0, NUM_TEST_IMAGES, TEST_IMAGES_FILE_PATH);
    MatrixXd Y = get_labels(0, NUM_TEST_IMAGES, TEST_LABELS_FILE_PATH);

    // Extract the weights and biases from file
    weights_and_biases wab;
    wab.W1 = MatrixXd::Random(L1_SIZE, 784) / 2;
    wab.B1 = MatrixXd::Random(L1_SIZE, 1) / 2;
    wab.W2 = MatrixXd::Random(L2_SIZE, L1_SIZE) / 2;
    wab.B2 = MatrixXd::Random(L2_SIZE, 1) / 2;
    wab.W3 = MatrixXd::Random(10, L2_SIZE) / 2;
    wab.B3 = MatrixXd::Random(10, 1) / 2;

    std::streamoff read_position = 0;
    read_position = read(&wab.W1, read_position, custom_weights_and_biases_file_path);
    read_position = read(&wab.B1, read_position, custom_weights_and_biases_file_path);
    read_position = read(&wab.W2, read_position, custom_weights_and_biases_file_path);
    read_position = read(&wab.B2, read_position, custom_weights_and_biases_file_path);
    read_position = read(&wab.W3, read_position, custom_weights_and_biases_file_path);
    read(&wab.B3, read_position, custom_weights_and_biases_file_path);

    // Do forward propagation with the stored weights and biases
    states_and_activations fp = forward_prop(X, wab);

    // Get the number of correct predictions
    int count = get_num_correct(get_predictions(fp.A3, NUM_TEST_IMAGES), Y, NUM_TEST_IMAGES);

    // Optionally print out the test labels and images
    if (PRINT_LABELS_AND_IMAGES)
        print_batch(X, Y, NUM_TEST_IMAGES);

    // Print the accuracy of the trained neural network in a box with a border
    int boxLength = 20;
    printBoxLine(boxLength);
    printBoxRow("Accuracy:", boxLength);
    printBoxLine(boxLength);
    printBoxRow(std::to_string(count) + "/" + std::to_string(NUM_TEST_IMAGES), boxLength);
    printBoxLine(boxLength);
    std::cout << RESET_TEXT << "\n";
}

void simulate_fed_avg()
{

    for (int i = 0; i < 5; i++)
    {
        std::cout << "Starting first learning cycle";

        std::vector<std::thread> threads;

        // Create threads for each training
        threads.emplace_back([]()
                             { train_network("./data/results_iid/aggregated.bin", "./data/results_iid/wb_client_0.bin", "./data/iid/train-images-0.idx3-ubyte", "./data/iid/train-labels-0.idx1-ubyte"); });
        threads.emplace_back([]()
                             { train_network("./data/results_iid/aggregated.bin", "./data/results_iid/wb_client_1.bin", "./data/iid/train-images-1.idx3-ubyte", "./data/iid/train-labels-1.idx1-ubyte"); });
        threads.emplace_back([]()
                             { train_network("./data/results_iid/aggregated.bin", "./data/results_iid/wb_client_2.bin", "./data/iid/train-images-2.idx3-ubyte", "./data/iid/train-labels-2.idx1-ubyte"); });

        // Wait for all threads to finish
        for (auto &thread : threads)
        {
            thread.join();
        }

        federatedAvg("./data/results_iid/wb_client_", "./data/results_iid/aggregated.bin", 3);
        std::cout << "Cycle " << i << " completed\n";
        runTest();
    }
}

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
            train_network("./data/results_iid/aggregated.bin", "./data/results_iid/aggregated.bin", "./data/iid/train-images-0.idx3-ubyte", "./data/iid/train-labels-0.idx1-ubyte");
            return 0;
        }
        else if (std::string(argv[1]) == "simulate")
        {
            simulate_fed_avg();
            return 0;
        }
        else if (std::string(argv[1]) == "get_random_wb")
        {
            saveRandomWBasFile();
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
