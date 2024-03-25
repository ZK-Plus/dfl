#include <fstream>
#include <iostream>
#include <chrono>
#include <Eigen/Dense>
#include "functions.h"

void extractNumbers()
{
    // open old file
    std::ifstream images_file(TRAIN_IMAGES_FILE_PATH, std::ios::binary);
    std::ifstream labels_file(TRAIN_LABELS_FILE_PATH, std::ios::binary);

    // open new file
    const std::string NEW_TRAIN_IMAGES_FILE_PATH = "./data/new_train-images.idx3-ubyte";
    const std::string NEW_TRAIN_LABELS_FILE_PATH = "./data/new_train-labels.idx1-ubyte";

    std::ofstream new_images_file(NEW_TRAIN_IMAGES_FILE_PATH, std::ios::binary);
    std::ofstream new_labels_file(NEW_TRAIN_LABELS_FILE_PATH, std::ios::binary);

    // check if files are open
    if (!images_file.is_open() || !labels_file.is_open() || !new_images_file.is_open() || !new_labels_file.is_open())
    {
        std::cout << "Error: Failed to open file" << std::endl;
    }

    // iterate over label file and extract indices with zeros
    // Skip headers
    labels_file.seekg(8); // MNIST label files have an 8-byte header

    // Read and process labels
    int zeroCount = 0;
    int label;
    std::vector<int> zeroIndices;
    int i = 0;
    while (labels_file.read(reinterpret_cast<char *>(&label), 1))
    { // Read until end of file
        std::cout << label << std::endl;
        if (label == 0)
        {
            zeroIndices.push_back(i); // Store index if label is 0
            zeroCount += 1;
        }
        ++i;
    }

    // Print number of zeros
    std::cout << "Number of zeros: " << zeroCount << std::endl;

    int magicNumber = 2049; // Typical magic number for MNIST label files
    int numberOfItems = zeroCount;
    magicNumber = __builtin_bswap32(magicNumber);     // Convert to big endian
    numberOfItems = __builtin_bswap32(numberOfItems); // Convert to big endian
    new_labels_file.write(reinterpret_cast<char *>(&magicNumber), sizeof(magicNumber));
    new_labels_file.write(reinterpret_cast<char *>(&numberOfItems), sizeof(numberOfItems));

    // Write the labels (all zeros)
    for (int i = 0; i < zeroCount; ++i)
    {
        char zeroLabel = 0;
        new_labels_file.write(&zeroLabel, 1);
    }

    // Close files
    images_file.close();
    labels_file.close();
    new_images_file.close();
    new_labels_file.close();
}

void verifyNewLabelsFile(const std::string &labelsFilePath, int expectedZerosCount, int filterNumber)
{
    std::ifstream labelsFile(labelsFilePath, std::ios::binary);
    if (!labelsFile.is_open())
    {
        std::cout << "Error: Failed to open file " << labelsFilePath << std::endl;
        return;
    }

    // Skip the header (8 bytes)
    labelsFile.seekg(8);

    int zeroCount = 0;
    char label;
    while (labelsFile.read(&label, 1))
    {
        // Assuming all labels in the new file are supposed to be zero
        if (label == filterNumber)
        {
            zeroCount++;
        }
    }

    std::cout << "Number of " << filterNumber << " in the file: " << zeroCount << std::endl;
    if (zeroCount == expectedZerosCount)
    {
        std::cout << "Verification Successful: The file contains the correct amount of zeros." << std::endl;
    }
    else
    {
        std::cout << "Verification Failed: The file does not contain the correct amount of zeros." << std::endl;
    }

    labelsFile.close();
}

int main()
{
    // extractNumbers();
    int expectedZerosCount = 5923; // Update the expected count as necessary
    //  Path to the new labels file
    const std::string NEW_TRAIN_LABELS_FILE_PATH = "./data/new_train-labels.idx1-ubyte";
    // Call the verifyNewLabelsFile method with the path and expected number of zeros
    verifyNewLabelsFile(NEW_TRAIN_LABELS_FILE_PATH, 5923, 0); // Update the expected count as necessary

    return 0;
}