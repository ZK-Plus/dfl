#include <fstream>
#include <iostream>
#include <chrono>
#include <Eigen/Dense>
#include <csignal>
#include "../functions.h"
#include "train-network2.h"
#include "network2.h"

using namespace std;
using Eigen::MatrixXd;

// Initialize weights and biases to a random value between -0.5 and 0.5
weights_and_biases wab;

void save_weights_and_biases()
{
    cout << "Saving weights and biases to file...\n";
    streamoff write_position = 0;
    write_position = save(wab.W1, write_position, WEIGHTS_AND_BIASES_FILE_PATH);
    write_position = save(wab.B1, write_position, WEIGHTS_AND_BIASES_FILE_PATH);
    write_position = save(wab.W2, write_position, WEIGHTS_AND_BIASES_FILE_PATH);
    write_position = save(wab.B2, write_position, WEIGHTS_AND_BIASES_FILE_PATH);
    write_position = save(wab.W3, write_position, WEIGHTS_AND_BIASES_FILE_PATH);
    save(wab.B3, write_position, WEIGHTS_AND_BIASES_FILE_PATH);

    // cout << "Biases of the second layer (B2):\n"
    //      << wab.B2 << "\n\n";
    // cout << "Length of the B2 matrix: " << wab.B2.rows() << "x" << wab.B2.cols() << "\n\n";
}

void save_weight_and_biases_as_csv()
{
    ofstream file;
    file.open("weights_and_biases.csv");

    file << "W1\n"
         << wab.W1 << "\n\n";
    file << "B1\n"
         << wab.B1 << "\n\n";
    file << "W2\n"
         << wab.W2 << "\n\n";
    file << "B2\n"
         << wab.B2 << "\n\n";
    file << "W3\n"
         << wab.W3 << "\n\n";
    file << "B3\n"
         << wab.B3 << "\n\n";

    file.close();
}

void read_weights_and_biases()
{
    cout << "Reading weights and biases from file...\n";

    // check if the file exists
    ifstream file(WEIGHTS_AND_BIASES_FILE_PATH);
    if (file.good())
    {
        file.close();
        streamoff read_position = 0;
        read_position = read(&wab.W1, read_position, WEIGHTS_AND_BIASES_FILE_PATH);
        read_position = read(&wab.B1, read_position, WEIGHTS_AND_BIASES_FILE_PATH);
        read_position = read(&wab.W2, read_position, WEIGHTS_AND_BIASES_FILE_PATH);
        read_position = read(&wab.B2, read_position, WEIGHTS_AND_BIASES_FILE_PATH);
        read_position = read(&wab.W3, read_position, WEIGHTS_AND_BIASES_FILE_PATH);
        read(&wab.B3, read_position, WEIGHTS_AND_BIASES_FILE_PATH);
    }
}

// ensures that the program saves the weights and biases to a file when the user presses Ctrl+C
void signal_callback_handler(int signum)
{
    // Optionally save weights and biases to file
    if (SAVE_WEIGHTS_AND_BIASES)
    {
        save_weights_and_biases();
    }

    exit(signum);
}

int train_network()
{
    // Register signal handler
    signal(SIGINT, signal_callback_handler);

    // Randomize the starting seed
    srand((unsigned int)time(nullptr));

    // Initialize weights and biases to a random value between -0.5 and 0.5
    wab.W1 = MatrixXd::Random(L1_SIZE, 784) / 2;
    wab.B1 = MatrixXd::Random(L1_SIZE, 1) / 2;
    wab.W2 = MatrixXd::Random(L2_SIZE, L1_SIZE) / 2;
    wab.B2 = MatrixXd::Random(L2_SIZE, 1) / 2;
    wab.W3 = MatrixXd::Random(10, L2_SIZE) / 2;
    wab.B3 = MatrixXd::Random(10, 1) / 2;

    // Initialize weights and biases by reading from file
    if (SAVE_WEIGHTS_AND_BIASES)
    {
        read_weights_and_biases();
    }

    // For each epoch, perform gradient descent and update weights and biases
    for (int epoch = 1; epoch <= NUM_EPOCHS; epoch++)
    {
        // Get start time
        auto start = chrono::high_resolution_clock::now();

        // Store number of correct predictions
        int count = gradient_descent(wab, LEARNING_RATE, epoch);

        // Get end time
        auto end = chrono::high_resolution_clock::now();

        // Calculate duration of time passed
        double duration = (double)chrono::duration_cast<chrono::microseconds>(end - start).count() / 1000000.0;

        // Calculate remaining time
        int seconds = (int)duration * (NUM_EPOCHS - epoch);
        int minutes = seconds / 60;
        int hours = minutes / 60;
        minutes %= 60;
        seconds %= 60;

        // Print the results of the epoch
        cout << "Epoch: " << epoch << "/" << NUM_EPOCHS << "\n";
        cout << "Accuracy: " << count << "/" << NUM_TRAIN_IMAGES << "\n";
        cout << "Time taken: " << duration << " seconds \n";
        cout << "Estimated time remaining: ";
        printf("%02d:%02d:%02d\n", hours, minutes, seconds);
        cout << "\n";
    }

    cout << "Finished training!\n";

    // Optionally save weights and biases to file
    if (SAVE_WEIGHTS_AND_BIASES)
    {
        save_weights_and_biases();
        save_weight_and_biases_as_csv();
    }

    return 0;
}
