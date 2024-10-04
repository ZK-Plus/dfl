#include <fstream>
#include <iostream>
#include "functions.h"
#include <Eigen/Dense>
#include <cfloat>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <string>
#include <chrono>
#include <thread>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
const int AES_KEY_LENGTH = 256;

// Function to load the private key from a PEM file
EVP_PKEY *loadPrivateKey(const char *privateKeyPath)
{
    FILE *privKeyFile = fopen(privateKeyPath, "r");
    if (!privKeyFile)
    {
        cerr << "Error: Unable to open private key file" << endl;
        return nullptr;
    }

    EVP_PKEY *privKey = PEM_read_PrivateKey(privKeyFile, NULL, NULL, NULL);
    fclose(privKeyFile);

    if (!privKey)
    {
        cerr << "Error: Unable to load private key" << endl;
        return nullptr;
    }

    return privKey;
}

// Function to load the public key from a PEM file
EVP_PKEY *loadPublicKey(const char *publicKeyPath)
{
    FILE *pubKeyFile = fopen(publicKeyPath, "r");
    if (!pubKeyFile)
    {
        cerr << "Error: Unable to open public key file" << endl;
        return nullptr;
    }

    EVP_PKEY *pubKey = PEM_read_PUBKEY(pubKeyFile, NULL, NULL, NULL);
    fclose(pubKeyFile);

    if (!pubKey)
    {
        cerr << "Error: Unable to load public key" << endl;
        return nullptr;
    }

    return pubKey;
}

// Function to print detailed OpenSSL error
void printOpenSSLError()
{
    unsigned long errCode = ERR_get_error();
    char errMsg[256];
    ERR_error_string_n(errCode, errMsg, sizeof(errMsg));
    cerr << "OpenSSL Error: " << errMsg << endl;
}

// Function to create a digital signature of the data
void signGlobalModel(const MatrixXd &model, EVP_PKEY *privateKey, string &signature)
{
    cout << "Signing global model..." << endl;
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        cerr << "Error: Failed to create EVP_MD_CTX" << endl;
        return;
    }

    // Initialize the signing context
    if (EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, privateKey) != 1)
    {
        cerr << "Error: EVP_DigestSignInit failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return;
    }

    // Update the signing context with the model data
    for (int i = 0; i < model.size(); ++i)
    {
        if (EVP_DigestSignUpdate(mdctx, &model(i), sizeof(model(i))) != 1)
        {
            cerr << "Error: EVP_DigestSignUpdate failed" << endl;
            printOpenSSLError();
            EVP_MD_CTX_free(mdctx);
            return;
        }
    }

    // Finalize the signature
    size_t sigLen = 0;
    if (EVP_DigestSignFinal(mdctx, NULL, &sigLen) != 1)
    {
        cerr << "Error: EVP_DigestSignFinal (getting signature size) failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return;
    }

    signature.resize(sigLen);
    if (EVP_DigestSignFinal(mdctx, (unsigned char *)signature.data(), &sigLen) != 1)
    {
        cerr << "Error: EVP_DigestSignFinal (signing) failed" << endl;
        printOpenSSLError();
    }

    EVP_MD_CTX_free(mdctx);
    cout << "Global model signed successfully." << endl;
}

// Function to encrypt the global model using AES encryption
void encryptGlobalModel(const MatrixXd &model, string &encryptedData, const unsigned char *key, const unsigned char *iv)
{
    cout << "Encrypting global model..." << endl;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        cerr << "Error: Failed to create EVP_CIPHER_CTX" << endl;
        return;
    }

    // Initialize the encryption context for AES-256 CBC
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
    {
        cerr << "Error: EVP_EncryptInit_ex failed" << endl;
        printOpenSSLError();
        EVP_CIPHER_CTX_free(ctx);
        return;
    }

    // Encrypt the model
    int len;
    unsigned char outbuf[1024];
    encryptedData.clear();

    for (int i = 0; i < model.size(); ++i)
    {
        if (EVP_EncryptUpdate(ctx, outbuf, &len, reinterpret_cast<const unsigned char *>(&model(i)), sizeof(model(i))) != 1)
        {
            cerr << "Error: EVP_EncryptUpdate failed" << endl;
            printOpenSSLError();
            EVP_CIPHER_CTX_free(ctx);
            return;
        }
        encryptedData.append(reinterpret_cast<char *>(outbuf), len);
    }

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, outbuf, &len) != 1)
    {
        cerr << "Error: EVP_EncryptFinal_ex failed" << endl;
        printOpenSSLError();
    }
    encryptedData.append(reinterpret_cast<char *>(outbuf), len);

    EVP_CIPHER_CTX_free(ctx);
    cout << "Global model encrypted successfully." << endl;
}

void verifySignature()
{
    cout << "Verifying signature..." << endl;
    // Load the public key
    EVP_PKEY *publicKey = loadPublicKey("public_key.pem");
    if (!publicKey)
    {
        cerr << "Error: Could not load public key." << endl;
        return;
    }

    // Initialize OpenSSL context
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL)
    {
        cerr << "Error: Failed to create EVP_MD_CTX" << endl;
        EVP_PKEY_free(publicKey);
        return;
    }

    // Use SHA-256 for signature verification
    const EVP_MD *md = EVP_sha256();

    // Initialize the verification context with the public key
    if (EVP_DigestVerifyInit(mdctx, NULL, md, NULL, publicKey) != 1)
    {
        cerr << "Error: DigestVerifyInit failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(publicKey);
        return;
    }

    // Simulate data processing (in a real case, you would hash the data)
    string data = "Simulated data";
    if (EVP_DigestVerifyUpdate(mdctx, data.c_str(), data.length()) != 1)
    {
        cerr << "Error: DigestVerifyUpdate failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(publicKey);
        return;
    }

    // Static valid signature for simulation (this should match a signature that could be verified with the public key)
    unsigned char staticValidSignature[] = {
        0x30, 0x45, 0x02, 0x20, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x02, 0x21,
        0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33, 0x44, 0x55,
        0x66, 0x77, 0x88, 0x99, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11,
        0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    size_t sigLen = sizeof(staticValidSignature);

    // Simulate the signature verification process
    int verificationResult = EVP_DigestVerifyFinal(mdctx, staticValidSignature, sigLen);

    // Cleanup
    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(publicKey);
}

void simulateKeyFetch()
{
    // Simulate the key fetch from GMC
    std::this_thread::sleep_for(std::chrono::milliseconds(550));
}

// Function to simulate the decryption of a 1.3 MB file
void simulateDecryption()
{

    // Simulate a computational delay (for instance, assume decryption takes 10 milliseconds)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void federatedAvg(const string &pathToFiles, const string outputPath, int numFiles)
{

    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    simulateKeyFetch();

    // Simulate a private key for signing
    EVP_PKEY *privateKey = loadPrivateKey("private_key.pem");
    if (!privateKey)
    {
        cerr << "Error: Could not load private key." << endl;
        return;
    }

    // Initialize the weights and biases to zero
    MatrixXd W1 = MatrixXd::Zero(200, 784);
    MatrixXd B1 = MatrixXd::Zero(200, 1);
    MatrixXd W2 = MatrixXd::Zero(50, 200);
    MatrixXd B2 = MatrixXd::Zero(50, 1);
    MatrixXd W3 = MatrixXd::Zero(10, 50);
    MatrixXd B3 = MatrixXd::Zero(10, 1);

    // Initialize the sum of weights and biases to zero
    MatrixXd sumW1 = MatrixXd::Zero(200, 784);
    MatrixXd sumB1 = MatrixXd::Zero(200, 1);
    MatrixXd sumW2 = MatrixXd::Zero(50, 200);
    MatrixXd sumB2 = MatrixXd::Zero(50, 1);
    MatrixXd sumW3 = MatrixXd::Zero(10, 50);
    MatrixXd sumB3 = MatrixXd::Zero(10, 1);

    int staticFile = 0;

    // Read the weights and biases from each file and add them to the sum
    for (int i = 0; i < numFiles; i++)
    {

        // additional overhead
        verifySignature();
        simulateDecryption();

        // Read the weights and biases from the file
        MatrixXd w1 = MatrixXd::Zero(200, 784);
        MatrixXd b1 = MatrixXd::Zero(200, 1);
        MatrixXd w2 = MatrixXd::Zero(50, 200);
        MatrixXd b2 = MatrixXd::Zero(50, 1);
        MatrixXd w3 = MatrixXd::Zero(10, 50);
        MatrixXd b3 = MatrixXd::Zero(10, 1);

        streamoff position = 0;
        position = read(&w1, position, pathToFiles + "0" + ".bin");
        position = read(&b1, position, pathToFiles + "0" + ".bin");
        position = read(&w2, position, pathToFiles + "0" + ".bin");
        position = read(&b2, position, pathToFiles + "0" + ".bin");
        position = read(&w3, position, pathToFiles + "0" + ".bin");
        read(&b3, position, pathToFiles + "0" + ".bin");

        // Add the weights and biases to the sum
        sumW1 += w1;
        sumB1 += b1;
        sumW2 += w2;
        sumB2 += b2;
        sumW3 += w3;
        sumB3 += b3;
    }

    // Calculate the average of the weights and biases
    W1 = sumW1 / numFiles;
    B1 = sumB1 / numFiles;
    W2 = sumW2 / numFiles;
    B2 = sumB2 / numFiles;
    W3 = sumW3 / numFiles;
    B3 = sumB3 / numFiles;

    // save the new weights and biases to the output file
    streamoff write_position = 0;
    write_position = save(W1, write_position, outputPath);
    write_position = save(B1, write_position, outputPath);
    write_position = save(W2, write_position, outputPath);
    write_position = save(B2, write_position, outputPath);
    write_position = save(W3, write_position, outputPath);
    save(B3, write_position, outputPath);

    // Encrypt the global model
    string encryptedModel;
    unsigned char aesKey[AES_KEY_LENGTH / 8]; // AES-256 key
    unsigned char iv[16];                     // Initialization vector
    RAND_bytes(aesKey, sizeof(aesKey));       // Generate random AES key
    RAND_bytes(iv, sizeof(iv));               // Generate random IV
    encryptGlobalModel(W1, encryptedModel, aesKey, iv);

    // Sign the global model
    string signature;
    signGlobalModel(W1, privateKey, signature);

    EVP_PKEY_free(privateKey);
    EVP_cleanup();
    ERR_free_strings();

    // additional overhead
    // signData();

    cout << "Federated averaging complete\n";
}

streamoff save(const MatrixXd &X, streamoff position, const string &path)
{
    // Get number of rows and columns
    int rows = (int)X.rows();
    int cols = (int)X.cols();

    // Declare file
    ofstream file;

    // Open file
    if (position == 0)
    {
        file = ofstream(path, ios::out | ios::binary);
    }
    else
    {
        file = ofstream(path, ios::app | ios::binary);
    }

    if (file.is_open())
    {
        // Save matrix X into the offset position
        file.seekp(position);
        for (int j = 0; j < cols; j++)
        {
            for (int i = 0; i < rows; i++)
            {
                file.write((char *)&X(i, j), sizeof(double));
            }
        }
        // Save the resulting position
        position = file.tellp();

        // Close the file
        file.close();
    }
    else
    {
        cout << "Error: Failed to open file WANDB";
        exit(1);
    }

    return position;
}

streamoff read(MatrixXd *X, streamoff position, const string &path)
{
    // Get number of rows and columns
    int rows = (int)(*X).rows();
    int cols = (int)(*X).cols();

    // Open file
    ifstream file(path, ios::in | ios::binary);

    if (file.is_open())
    {
        // Extract matrix X from offset position
        file.seekg(position);

        double temp = 0;
        for (int j = 0; j < cols; j++)
        {
            for (int i = 0; i < rows; i++)
            {
                file.read((char *)&temp, sizeof(double));
                (*X)(i, j) = temp;
            }
        }
        // Save the resulting position
        position = file.tellg();

        // Close the file
        file.close();
    }
    else
    {
        cout << "Error: Failed to open file WANDB";
        exit(1);
    }

    return position;
}

MatrixXd get_labels(int offset, int size, const string &path)
{
    // Create Y Matrix of dimension 10 x size
    MatrixXd Y = MatrixXd::Zero(10, size);

    // Open file
    ifstream labels_file(path, ios::in | ios::binary);

    if (labels_file.is_open())
    {
        // Extract matrix Y by reading size number of labels from offset of beginning of the file
        labels_file.seekg(LABEL_START + offset);
        int temp = 0;
        for (int i = 0; i < size; i++)
        {
            labels_file.read((char *)&temp, 1);
            Y(temp, i) = 1;
        }
        // Close the file
        labels_file.close();
    }
    else
    {
        cout << "Error: Failed to open file " << path << endl;
        exit(1);
    }

    return Y;
}

Eigen::MatrixXd get_images(int offset, int size, const string &path)
{
    // Create X Matrix of dimension 784 x size to represent input layer
    MatrixXd X = MatrixXd::Zero(784, size);

    // Open file
    ifstream images_file(path, ios::in | ios::binary);

    if (images_file.is_open())
    {
        // Extract matrix X by reading size number of images from offset of beginning of the file
        images_file.seekg(IMAGE_START + offset);
        int temp = 0;
        for (int i = 0; i < 784 * size; i++)
        {
            images_file.read((char *)&temp, 1);

            // Transform temp from range [0, 255] to range [-1, 1]
            double transform = (temp - 127.5) / 127.5;

            X(i % 784, i / 784) = transform;
        }
        // Close the file
        images_file.close();
    }
    else
    {
        cout << "Error: Failed to open file " << path << endl;
        exit(1);
    }

    return X;
}

void print_batch(const MatrixXd &X, const MatrixXd &Y, int size)
{
    // For size number of labels/images, print them
    for (int i = 0; i < size; i++)
    {
        // Print label
        cout << "The following number is: ";
        for (int j = 0; j < 10; j++)
        {
            if (Y(j, i) == 1)
            {
                cout << j << "\n";
                break;
            }
        }
        // Print image
        for (int j = 0; j < 784; j++)
        {
            if (j != 0 && j % 28 == 0)
            {
                cout << "\n";
            }
            if (X(j, i) < 0)
            {
                cout << "@.@"; // Represents dark pixel
            }
            else
            {
                cout << " . "; // Represents light pixel
            }
        }
        cout << "\n";
    }
}

MatrixXd softmax(const MatrixXd &Z)
{
    // Convert into array
    MatrixXd Z1 = Z.array();

    // Find max values of each column
    VectorXd Max = Z1.colwise().maxCoeff();

    // Subtract max, compute exponential, compute sum, and then compute logarithm
    MatrixXd Z2 = (Z1.rowwise() - Max.transpose()).array().exp().colwise().sum().array().log();

    // Compute offset
    VectorXd Offset = Z2.transpose() + Max;

    // Subtract offset and compute exponential
    return (Z1.rowwise() - Offset.transpose()).array().exp();
}

MatrixXd deriv_tanh(const MatrixXd &Z)
{
    return 1 - Z.array().tanh().pow(2);
}

double ReLU(double x)
{
    if (x > 0)
        return x;
    else
        return 0;
}

double deriv_ReLU(double x)
{
    return x > 0;
}

double leaky_ReLU(double x)
{
    if (x > 0)
        return x;
    else
        return 0.01 * x;
}

double deriv_leaky_ReLU(double x)
{
    if (x > 0)
        return 1;
    else
        return 0.01;
}

MatrixXd get_predictions(const MatrixXd &AL, int size)
{
    // Initialize matrix of predictions
    MatrixXd P = MatrixXd::Zero(10, size);

    // For each column of AL, find its largest value and fill its position in P 1. Leave the rest as 0.
    // Essentially taking the argmax to find the prediction
    for (int i = 0; i < size; i++)
    {
        double largest = -DBL_MAX;
        int prediction = -1;
        for (int j = 0; j < 10; j++)
        {
            if (AL(j, i) > largest)
            {
                prediction = j;
                largest = AL(j, i);
            }
        }
        P(prediction, i) = 1;
    }

    return P;
}

int get_num_correct(const MatrixXd &P, const MatrixXd &Y, int size)
{
    // Initialize variable to store number of correct predictions
    int correct = 0;

    // For size number of columns, compare position of 1's. If they match, it's a correct prediction.
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            if (P(j, i) == 1)
            {
                if (Y(j, i) == 1)
                    correct++;
                break;
            }
        }
    }

    return correct;
}

MatrixXd get_label_batch(const int offsets[], int index, int size, const string &path)
{
    // Create Y Matrix of dimension 10 x size
    MatrixXd Y = MatrixXd::Zero(10, size);

    // Open file
    ifstream labels_file(path, ios::in | ios::binary);

    if (labels_file.is_open())
    {
        // Extract size number of random labels
        for (int i = 0; i < size; i++)
        {
            labels_file.seekg(LABEL_START + offsets[index + i]);
            int temp = 0;
            labels_file.read((char *)&temp, 1);
            Y(temp, i) = 1;
            // print the label to console
            // cout << "The following number is: " << temp << endl;
        }

        // Close the file
        labels_file.close();
    }
    else
    {
        cout << "Error: Failed to open file " << path << endl;
        exit(1);
    }

    return Y;
}

MatrixXd get_image_batch(const int offsets[], int index, int size, const string &path)
{
    // Create X Matrix of dimension 784 x size to represent input layer
    MatrixXd X = MatrixXd::Zero(784, size);

    // Open file
    ifstream images_file(path, ios::in | ios::binary);

    if (images_file.is_open())
    {
        // Extract size number of random images
        for (int i = 0; i < size; i++)
        {
            images_file.seekg(IMAGE_START + 784 * offsets[index + i]);
            for (int j = 0; j < 784; j++)
            {
                int temp = 0;
                images_file.read((char *)&temp, 1);

                // Transform temp from range [0, 255] to range [-1, 1]
                double transform = (temp - 127.5) / 127.5;

                X(j % 784, i) = transform;
            }
        }
        // Close the file
        images_file.close();
    }
    else
    {
        cout << "Error: Failed to open file " << path << endl;
        exit(1);
    }

    return X;
}