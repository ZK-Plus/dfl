#include <fstream>
#include <iostream>
#include <chrono>
#include <random>
#include <Eigen/Dense>
#include "functions.h"
#include <algorithm>

void printImages(int count)
{
    std::ifstream images_file("./data/iid/train-images-0.idx3-ubyte", std::ios::binary);
    std::ifstream labels_file("./data/iid/train-labels-0.idx1-ubyte", std::ios::binary);
    if (!images_file.is_open() || !labels_file.is_open())
    {
        std::cerr << "Error: Failed to open files" << std::endl;
        return;
    }

    // Skip the headers
    images_file.seekg(16);
    labels_file.seekg(8);

    char image[28 * 28];
    unsigned char label; // Use unsigned char for the label to properly read values 0-255
    for (int i = 0; i < count; i++)
    {
        if (!labels_file.read(reinterpret_cast<char *>(&label), sizeof(label)))
        {
            std::cerr << "Error reading label from file" << std::endl;
            return;
        }

        // Print the label before the image
        std::cout << "Label: " << static_cast<int>(label) << std::endl;

        if (!images_file.read(reinterpret_cast<char *>(&image), sizeof(image)))
        {
            std::cerr << "Error reading image from file" << std::endl;
            return;
        }

        for (int j = 0; j < 784; j++)
        {
            if (j != 0 && j % 28 == 0)
            {
                std::cout << '\n';
            }
            std::cout << (static_cast<unsigned char>(image[j]) < 128 ? " . " : "@@@");
        }
        std::cout << "\n\n"; // Extra newline for spacing between images
    }

    images_file.close();
    labels_file.close();
}

void printLabels(int count)
{
    std::ifstream labels_file("./data/iid/train-labels-0.idx1-ubyte", std::ios::binary);
    if (!labels_file.is_open())
    {
        std::cout << "Error: Failed to open file" << std::endl;
        return;
    }

    // Skip headers
    labels_file.seekg(8);

    // Read and print labels
    char label;
    for (int i = 0; i < count; i++)
    {
        if (!labels_file.read(&label, 1))
        {
            std::cerr << "Error reading from file" << std::endl;
            return;
        }
        std::cout << static_cast<int>(label) << std::endl;
    }

    labels_file.close();
}

void prepareIID(int number_of_partitions, int sample_size)
{
    std::ifstream images_file("./data/train-images.idx3-ubyte", std::ios::binary);
    std::ifstream labels_file("./data/train-labels.idx1-ubyte", std::ios::binary);

    if (!images_file.is_open() || !labels_file.is_open())
    {
        std::cout << "Error: Failed to open original image and label file" << std::endl;
        return;
    }

    // Skip headers for the original files just once, since we will manage offsets manually
    images_file.seekg(16, std::ios::beg);
    labels_file.seekg(8, std::ios::beg);

    for (int i = 0; i < number_of_partitions; i++)
    {
        const std::string NEW_TRAIN_IMAGES_FILE_PATH = "./data/iid/train-images-" + std::to_string(i) + ".idx3-ubyte";
        const std::string NEW_TRAIN_LABELS_FILE_PATH = "./data/iid/train-labels-" + std::to_string(i) + ".idx1-ubyte";

        std::ofstream new_images_file(NEW_TRAIN_IMAGES_FILE_PATH, std::ios::binary);
        std::ofstream new_labels_file(NEW_TRAIN_LABELS_FILE_PATH, std::ios::binary);

        if (!new_images_file.is_open() || !new_labels_file.is_open())
        {
            std::cout << "Error: Failed to open new file" << std::endl;
            return;
        }

        // Prepare headers for the new files
        int magicNumberImages = 2051;
        int magicNumberLabels = 2049;
        int numberOfImages = sample_size;
        int numberOfRows = 28;
        int numberOfColumns = 28;

        // Convert to big endian
        magicNumberImages = __builtin_bswap32(magicNumberImages);
        magicNumberLabels = __builtin_bswap32(magicNumberLabels);
        numberOfImages = __builtin_bswap32(numberOfImages);
        numberOfRows = __builtin_bswap32(numberOfRows);
        numberOfColumns = __builtin_bswap32(numberOfColumns);

        // Write headers for the new files
        new_images_file.write(reinterpret_cast<char *>(&magicNumberImages), sizeof(magicNumberImages));
        new_images_file.write(reinterpret_cast<char *>(&numberOfImages), sizeof(numberOfImages));
        new_images_file.write(reinterpret_cast<char *>(&numberOfRows), sizeof(numberOfRows));
        new_images_file.write(reinterpret_cast<char *>(&numberOfColumns), sizeof(numberOfColumns));

        new_labels_file.write(reinterpret_cast<char *>(&magicNumberLabels), sizeof(magicNumberLabels));
        new_labels_file.write(reinterpret_cast<char *>(&numberOfImages), sizeof(numberOfImages));

        // Read and write the data for each partition
        char image[28 * 28];
        char label;

        for (int j = 0; j < sample_size; j++)
        {
            if (!images_file.read(reinterpret_cast<char *>(&image), sizeof(image)) ||
                !labels_file.read(&label, sizeof(label)))
            {
                std::cerr << "Error reading from source files" << std::endl;
                return;
            }
            new_images_file.write(reinterpret_cast<char *>(&image), sizeof(image));
            new_labels_file.write(&label, sizeof(label));
        }

        // Closing the newly created files for this partition
        new_images_file.close();
        new_labels_file.close();
    }

    // Closing the original dataset files
    images_file.close();
    labels_file.close();
}

void prepareNIID(int sample_size, int first_number, int second_number)
{
    // open old file
    std::ifstream images_file(TRAIN_IMAGES_FILE_PATH, std::ios::binary);
    std::ifstream labels_file(TRAIN_LABELS_FILE_PATH, std::ios::binary);

    if (!images_file.is_open() || !labels_file.is_open())
    {
        std::cout << "Error: Failed to open file" << std::endl;
        return;
    }

    images_file.seekg(16, std::ios::beg);
    labels_file.seekg(8, std::ios::beg);

    std::vector<int> first_number_indices;
    std::vector<int> second_number_indices;
    std::vector<int> merged_indices;

    // get indices of first and second number
    char label;
    int i = 0;
    while (labels_file.read(reinterpret_cast<char *>(&label), 1))
    { // Read until end of file
        // std::cout << label << std::endl;
        if (label == first_number)
        {
            first_number_indices.push_back(i);
        }
        else if (label == second_number)
        {
            second_number_indices.push_back(i);
        }
        ++i;
    }

    // shuffle indices randomly
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(first_number_indices.begin(), first_number_indices.end(), g);
    std::shuffle(second_number_indices.begin(), second_number_indices.end(), g);

    // merge the two indices
    for (int i = 0; i < sample_size; i++)
    {
        merged_indices.push_back(first_number_indices[i]);
        merged_indices.push_back(second_number_indices[i]);
    }

    // write indices to new file according to sample size
    const std::string NEW_LABELS_FILE_PATH = "./data/niid/train-labels" + std::to_string(first_number) + "_" + std::to_string(second_number) + ".idx1-ubyte";
    const std::string NEW_IMAGES_FILE_PATH = "./data/niid/train-images" + std::to_string(first_number) + "_" + std::to_string(second_number) + ".idx3-ubyte";
    std::ofstream new_labels_file(NEW_LABELS_FILE_PATH, std::ios::binary);
    std::ofstream new_images_file(NEW_IMAGES_FILE_PATH, std::ios::binary);

    if (!new_labels_file.is_open() || !new_images_file.is_open())
    {
        std::cout << "Error: Failed to open new files" << std::endl;
        return;
    }

    int magicNumber = 2049; // Typical magic number for MNIST label files
    int numberOfItems = sample_size;
    magicNumber = __builtin_bswap32(magicNumber);     // Convert to big endian
    numberOfItems = __builtin_bswap32(numberOfItems); // Convert to big endian
    new_labels_file.write(reinterpret_cast<char *>(&magicNumber), sizeof(magicNumber));
    new_labels_file.write(reinterpret_cast<char *>(&numberOfItems), sizeof(numberOfItems));

    magicNumber = 2051;                           // Typical magic number for MNIST image files
    magicNumber = __builtin_bswap32(magicNumber); // Convert to big endian
    new_images_file.write(reinterpret_cast<char *>(&magicNumber), sizeof(magicNumber));
    new_images_file.write(reinterpret_cast<char *>(&numberOfItems), sizeof(numberOfItems));
    int numberOfRows = 28;
    int numberOfColumns = 28;
    numberOfRows = __builtin_bswap32(numberOfRows);       // Convert to big endian
    numberOfColumns = __builtin_bswap32(numberOfColumns); // Convert to big endian
    new_images_file.write(reinterpret_cast<char *>(&numberOfRows), sizeof(numberOfRows));
    new_images_file.write(reinterpret_cast<char *>(&numberOfColumns), sizeof(numberOfColumns));

    // Write the labels and images
    char label_byte;
    char image[28 * 28];
    for (int i = 0; i < sample_size; i++)
    {
        // Write the label
        if (i % 2 == 0)
        {
            label_byte = first_number;
        }
        else
        {
            label_byte = second_number;
        }
        new_labels_file.write(&label_byte, 1);

        // Write the image
        images_file.seekg(16 + merged_indices[i] * 28 * 28);
        images_file.read(reinterpret_cast<char *>(&image), 28 * 28);
        new_images_file.write(reinterpret_cast<char *>(&image), 28 * 28);
    }

    // Close files
    images_file.close();
    labels_file.close();
    new_images_file.close();
    new_labels_file.close();
}

// non iid extraction
void extractNumbers()
{
    // open old file
    std::ifstream images_file(TRAIN_IMAGES_FILE_PATH, std::ios::binary);
    std::ifstream labels_file(TRAIN_LABELS_FILE_PATH, std::ios::binary);

    // open new file
    const std::string NEW_TRAIN_IMAGES_FILE_PATH = "./data/niid/new_train-images.idx3-ubyte";
    const std::string NEW_TRAIN_LABELS_FILE_PATH = "./data/nidd/new_train-labels.idx1-ubyte";

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
    // // extractNumbers();
    // int expectedZerosCount = 5923; // Update the expected count as necessary
    // //  Path to the new labels file
    // const std::string NEW_TRAIN_LABELS_FILE_PATH = "./data/new_train-labels.idx1-ubyte";
    // // Call the verifyNewLabelsFile method with the path and expected number of zeros
    // verifyNewLabelsFile(NEW_TRAIN_LABELS_FILE_PATH, 5923, 0); // Update the expected count as necessary
    prepareIID(12, 10000);
    printImages(100);
    //  prepareNIID(100, 0, 7);
    //  printImages(100);

    return 0;
}