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
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <cstdint>
#include <cstring>

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
                             { train_network("./data/results_iid/aggregated.bin", "./data/results_iid/wb_client_0.bin", "./data/iid/train-images-0.idx3-ubyte", "./data/iid/train-labels-0.idx1-ubyte", 30); });
        threads.emplace_back([]()
                             { train_network("./data/results_iid/aggregated.bin", "./data/results_iid/wb_client_1.bin", "./data/iid/train-images-1.idx3-ubyte", "./data/iid/train-labels-1.idx1-ubyte", 30); });
        threads.emplace_back([]()
                             { train_network("./data/results_iid/aggregated.bin", "./data/results_iid/wb_client_2.bin", "./data/iid/train-images-2.idx3-ubyte", "./data/iid/train-labels-2.idx1-ubyte", 30); });

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

static void printOpenSSLErrorServer()
{
    unsigned long errCode = ERR_get_error();
    char errMsg[256];
    ERR_error_string_n(errCode, errMsg, sizeof(errMsg));
    std::cerr << "OpenSSL Error: " << errMsg << std::endl;
}

static EVP_PKEY *loadPrivateKeyServer(const char *privateKeyPath)
{
    FILE *f = fopen(privateKeyPath, "r");
    if (!f)
    {
        std::cerr << "Error: Unable to open private key file: " << privateKeyPath << std::endl;
        return nullptr;
    }
    EVP_PKEY *k = PEM_read_PrivateKey(f, NULL, NULL, NULL);
    fclose(f);

    if (!k)
    {
        std::cerr << "Error: PEM_read_PrivateKey failed" << std::endl;
        printOpenSSLErrorServer();
        return nullptr;
    }
    return k;
}

static bool readU32(const std::string &buf, size_t &off, uint32_t &v)
{
    if (off + sizeof(uint32_t) > buf.size())
        return false;
    std::memcpy(&v, buf.data() + off, sizeof(uint32_t));
    off += sizeof(uint32_t);
    return true;
}

static bool rsaOaepSha256Decrypt(EVP_PKEY *privateKey, const std::string &in, std::string &out)
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(privateKey, nullptr);
    if (!ctx)
    {
        printOpenSSLErrorServer();
        return false;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0 ||
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0 ||
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) <= 0)
    {
        printOpenSSLErrorServer();
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    size_t outLen = 0;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outLen,
                         (const unsigned char *)in.data(), in.size()) <= 0)
    {
        printOpenSSLErrorServer();
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    out.resize(outLen);
    if (EVP_PKEY_decrypt(ctx, (unsigned char *)out.data(), &outLen,
                         (const unsigned char *)in.data(), in.size()) <= 0)
    {
        printOpenSSLErrorServer();
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    out.resize(outLen);
    EVP_PKEY_CTX_free(ctx);
    return true;
}

static bool decryptAes256Cbc(const std::string &ciphertext,
                            std::string &plaintext,
                            const unsigned char *key,
                            const unsigned char *iv)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        std::cerr << "Error: EVP_CIPHER_CTX_new failed" << std::endl;
        return false;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
    {
        printOpenSSLErrorServer();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    plaintext.clear();
    plaintext.resize(ciphertext.size() + AES_BLOCK_SIZE);

    int outLen1 = 0;
    if (EVP_DecryptUpdate(ctx,
                          (unsigned char *)plaintext.data(), &outLen1,
                          (const unsigned char *)ciphertext.data(), (int)ciphertext.size()) != 1)
    {
        printOpenSSLErrorServer();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int outLen2 = 0;
    if (EVP_DecryptFinal_ex(ctx,
                           (unsigned char *)plaintext.data() + outLen1, &outLen2) != 1)
    {
        printOpenSSLErrorServer();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    plaintext.resize((size_t)outLen1 + (size_t)outLen2);
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

// .enc package format from train_network:
// [u32 senderPubLen][senderPubDer][u32 wrappedLen][wrappedKeyIv][u32 ctLen][ciphertext]
static bool decryptEncPackageToPlain(const std::string &encPkg,
                                     EVP_PKEY *aggregatorPrivateKey,
                                     std::string &plainOut)
{
    size_t off = 0;

    uint32_t senderPubLen = 0;
    if (!readU32(encPkg, off, senderPubLen) || off + senderPubLen > encPkg.size())
        return false;
    off += senderPubLen; // currently unused

    uint32_t wrappedLen = 0;
    if (!readU32(encPkg, off, wrappedLen) || off + wrappedLen > encPkg.size())
        return false;
    std::string wrapped = encPkg.substr(off, wrappedLen);
    off += wrappedLen;

    uint32_t ctLen = 0;
    if (!readU32(encPkg, off, ctLen) || off + ctLen > encPkg.size())
        return false;
    std::string ciphertext = encPkg.substr(off, ctLen);
    off += ctLen;

    std::string keyIv;
    if (!rsaOaepSha256Decrypt(aggregatorPrivateKey, wrapped, keyIv))
        return false;
    if (keyIv.size() != 48)
        return false;

    unsigned char aesKey[32];
    unsigned char iv[16];
    std::memcpy(aesKey, keyIv.data(), 32);
    std::memcpy(iv, keyIv.data() + 32, 16);

    if (!decryptAes256Cbc(ciphertext, plainOut, aesKey, iv))
        return false;
    return true;
}

void start_zerompq_server(int number_clients, const std::string &aggregatorPrivateKeyPath = "private_key.pem")
{

    int counter = 0;
    zmq::context_t context{1};
    zmq::socket_t socket{context, zmq::socket_type::rep};
    socket.bind("tcp://*:5555");
    std::cout << "Server started with " << number_clients << " expected clients\n";

    EVP_PKEY *aggPrivKey = loadPrivateKeyServer(aggregatorPrivateKeyPath.c_str());
    if (!aggPrivKey)
    {
        std::cerr << "Error: could not load aggregator private key" << std::endl;
        return;
    }

    fs::create_directory("./received_models"); // Ensure the directory exists

    while (true && counter < number_clients)
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

        std::string plain;
        if (!decryptEncPackageToPlain(fileContents, aggPrivKey, plain))
        {
            std::cerr << "Error: decryptEncPackageToPlain failed for " << filename << std::endl;
            socket.send(zmq::str_buffer("Decrypt failed"), zmq::send_flags::none);
            continue;
        }

        std::cout << "Decryption OK (plaintext bytes=" << plain.size() << ")" << std::endl;

        // output filename: wb_client_X.enc -> wb_client_X.bin
        std::string outName = filename;
        if (outName.size() >= 4 && outName.substr(outName.size() - 4) == ".enc")
            outName = outName.substr(0, outName.size() - 4) + ".bin";

        // Save the decrypted file (same binary format as save())
        std::ofstream outFile("./received_models/" + outName, std::ios::binary);
        outFile.write(plain.data(), (std::streamsize)plain.size());
        outFile.close();
        counter++;

        std::cout << "Saved decrypted model to ./received_models/" << outName << std::endl;

        // Acknowledge file receipt
        socket.send(zmq::str_buffer("File received and decrypted"), zmq::send_flags::none);
    }

    EVP_PKEY_free(aggPrivKey);
}

void start_zerompq_client(const std::string serverIP, const std::string deviceID)
{
    zmq::context_t context{1};
    zmq::socket_t socket{context, zmq::socket_type::req};
    socket.connect("tcp://" + serverIP + ":5555");

    // Prepare the filename (server will save as .bin after decrypt)
    std::string filename = "wb_client_" + deviceID + ".enc";

    // Send the filename
    socket.send(zmq::buffer(filename), zmq::send_flags::none);

    // Wait for filename ack
    zmq::message_t reply;
    socket.recv(reply);
    std::cout << "Server: " << reply.to_string() << std::endl;

    // Read the encrypted package produced by train_network (wb_out + ".enc")
    std::ifstream file("./data/lm.bin.enc", std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open ./data/lm.bin.enc" << std::endl;
        return;
    }
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
            std::cout << "Starting server\n";
            const std::string privKeyPath = (argc > 3) ? argv[3] : "private_key.pem";
            start_zerompq_server(std::stoi(argv[2]), privKeyPath);
            return 0;
        }
        else if (std::string(argv[1]) == "client")
        {
            start_zerompq_client(argv[2], argv[3]);
            return 0;
        }
        else if (std::string(argv[1]) == "train")
        {
            if (argc < 4)
            {
                std::cerr << "Usage: train <epochs> <current_aggregator_public_rsa_key_der_hex>" << std::endl;
                return 1;
            }
            train_network("./data/random_start.bin", "./data/lm.bin", "./data/train-images.idx3-ubyte", "./data/train-labels.idx1-ubyte", std::stoi(argv[2]), argv[3]);
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
        else if (std::string(argv[1]) == "aggregate")
        {
            federatedAvg("./data/results_iid/wb_client_", "./data/results_iid/aggregated.bin", std::stoi(argv[2]));
            runTest();
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
