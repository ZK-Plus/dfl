#include <fstream>
#include <iostream>
#include "functions.h"
#include <Eigen/Dense>
#include <cfloat>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <cctype>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
const int AES_KEY_LENGTH = 256;

void printOpenSSLError();
EVP_PKEY *loadPrivateKey(const char *privateKeyPath);
EVP_PKEY *loadPublicKey(const char *publicKeyPath);
void signGlobalModel(const MatrixXd &model, EVP_PKEY *privateKey, string &signature);

static void appendMatrixAsSaveFormat(const MatrixXd &model, std::string &out)
{
    const int rows = (int)model.rows();
    const int cols = (int)model.cols();
    for (int j = 0; j < cols; j++)
    {
        for (int i = 0; i < rows; i++)
        {
            const double value = model(i, j);
            out.append(reinterpret_cast<const char *>(&value), sizeof(double));
        }
    }
}

static bool verifyGlobalModelSignatureWithKey(const MatrixXd &model,
                                              const std::string &signature,
                                              EVP_PKEY *publicKey)
{
    if (!publicKey)
        return false;

    std::string payload;
    payload.reserve((size_t)model.size() * sizeof(double));
    appendMatrixAsSaveFormat(model, payload);

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        cerr << "Error: Failed to create EVP_MD_CTX" << endl;
        return false;
    }

    bool ok = false;
    if (EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, publicKey) != 1)
    {
        cerr << "Error: EVP_DigestVerifyInit failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return false;
    }

    if (EVP_DigestVerifyUpdate(mdctx, payload.data(), payload.size()) != 1)
    {
        cerr << "Error: EVP_DigestVerifyUpdate failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return false;
    }

    const int rc = EVP_DigestVerifyFinal(mdctx,
                                         reinterpret_cast<const unsigned char *>(signature.data()),
                                         signature.size());
    if (rc == 1)
    {
        ok = true;
    }
    else if (rc == 0)
    {
        ok = false;
    }
    else
    {
        cerr << "Error: EVP_DigestVerifyFinal failed" << endl;
        printOpenSSLError();
        ok = false;
    }

    EVP_MD_CTX_free(mdctx);
    return ok;
}

static bool signBytesWithKey(EVP_PKEY *privateKey, const char *data, size_t len, std::string &signatureOut)
{
    if (!privateKey)
        return false;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        cerr << "Error: Failed to create EVP_MD_CTX" << endl;
        return false;
    }

    if (EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, privateKey) != 1)
    {
        cerr << "Error: EVP_DigestSignInit failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return false;
    }

    if (EVP_DigestSignUpdate(mdctx, data, len) != 1)
    {
        cerr << "Error: EVP_DigestSignUpdate failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return false;
    }

    size_t sigLen = 0;
    if (EVP_DigestSignFinal(mdctx, NULL, &sigLen) != 1)
    {
        cerr << "Error: EVP_DigestSignFinal (size) failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return false;
    }

    signatureOut.assign(sigLen, '\0');
    if (EVP_DigestSignFinal(mdctx, reinterpret_cast<unsigned char *>(signatureOut.data()), &sigLen) != 1)
    {
        cerr << "Error: EVP_DigestSignFinal (sign) failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        signatureOut.clear();
        return false;
    }

    signatureOut.resize(sigLen);
    EVP_MD_CTX_free(mdctx);
    return true;
}

static bool hexToBytes(const std::string &hexIn, std::vector<unsigned char> &out)
{
    std::string hex = hexIn;
    if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0)
        hex = hex.substr(2);

    std::string cleaned;
    cleaned.reserve(hex.size());
    for (char c : hex)
    {
        if (!std::isspace((unsigned char)c))
            cleaned.push_back(c);
    }

    if (cleaned.empty() || (cleaned.size() % 2 != 0))
        return false;

    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F')
            return 10 + (c - 'A');
        return -1;
    };

    out.clear();
    out.reserve(cleaned.size() / 2);
    for (size_t i = 0; i < cleaned.size(); i += 2)
    {
        const int hi = nibble(cleaned[i]);
        const int lo = nibble(cleaned[i + 1]);
        if (hi < 0 || lo < 0)
            return false;
        out.push_back((unsigned char)((hi << 4) | lo));
    }
    return true;
}

static EVP_PKEY *loadPublicKeyFromDerHex(const std::string &derHex)
{
    std::vector<unsigned char> der;
    if (!hexToBytes(derHex, der))
    {
        cerr << "Error: invalid DER-hex public key" << endl;
        return nullptr;
    }

    const unsigned char *p = der.data();
    EVP_PKEY *pub = d2i_PUBKEY(nullptr, &p, (long)der.size());
    if (!pub)
    {
        cerr << "Error: d2i_PUBKEY failed" << endl;
        printOpenSSLError();
        return nullptr;
    }
    return pub;
}

bool signModelToSignature(const MatrixXd &model,
                          const std::string &privateKeyPemPath,
                          std::string &signatureOut)
{
    EVP_PKEY *privateKey = loadPrivateKey(privateKeyPemPath.c_str());
    if (!privateKey)
        return false;

    signatureOut.clear();
    signGlobalModel(model, privateKey, signatureOut);
    EVP_PKEY_free(privateKey);

    return !signatureOut.empty();
}

bool verifyModelSignature(const MatrixXd &model,
                          const std::string &signature,
                          const std::string &publicKeyPemPath)
{
    EVP_PKEY *publicKey = loadPublicKey(publicKeyPemPath.c_str());
    if (!publicKey)
        return false;

    const bool ok = verifyGlobalModelSignatureWithKey(model, signature, publicKey);
    EVP_PKEY_free(publicKey);
    return ok;
}

bool verifyModelFileSignature(const std::string &modelPath,
                              const std::string &sigPath,
                              const std::string &publicKeyDerHex)
{
    std::ifstream modelFile(modelPath, std::ios::binary);
    if (!modelFile.is_open())
    {
        cerr << "Error: Could not open model file: " << modelPath << endl;
        return false;
    }
    std::string modelBytes((std::istreambuf_iterator<char>(modelFile)), std::istreambuf_iterator<char>());
    modelFile.close();

    std::ifstream sigFile(sigPath, std::ios::binary);
    if (!sigFile.is_open())
    {
        cerr << "Error: Could not open signature file: " << sigPath << endl;
        return false;
    }
    std::string sigBytes((std::istreambuf_iterator<char>(sigFile)), std::istreambuf_iterator<char>());
    sigFile.close();

    if (sigBytes.empty())
    {
        cerr << "Error: Signature file is empty: " << sigPath << endl;
        return false;
    }

    EVP_PKEY *publicKey = loadPublicKeyFromDerHex(publicKeyDerHex);
    if (!publicKey)
        return false;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        cerr << "Error: Failed to create EVP_MD_CTX" << endl;
        EVP_PKEY_free(publicKey);
        return false;
    }

    if (EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, publicKey) != 1)
    {
        cerr << "Error: EVP_DigestVerifyInit failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(publicKey);
        return false;
    }

    if (EVP_DigestVerifyUpdate(mdctx, modelBytes.data(), modelBytes.size()) != 1)
    {
        cerr << "Error: EVP_DigestVerifyUpdate failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(publicKey);
        return false;
    }

    const int rc = EVP_DigestVerifyFinal(mdctx,
                                         reinterpret_cast<const unsigned char *>(sigBytes.data()),
                                         sigBytes.size());

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(publicKey);

    if (rc == 1)
        return true;
    if (rc == 0)
        return false;

    cerr << "Error: EVP_DigestVerifyFinal failed" << endl;
    printOpenSSLError();
    return false;
}

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
    if (!privateKey)
    {
        cerr << "Error: privateKey is null" << endl;
        return;
    }

    std::string payload;
    payload.reserve((size_t)model.size() * sizeof(double));
    appendMatrixAsSaveFormat(model, payload);

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

    // Sign exactly the bytes written by save() (column-major doubles)
    if (EVP_DigestSignUpdate(mdctx, payload.data(), payload.size()) != 1)
    {
        cerr << "Error: EVP_DigestSignUpdate failed" << endl;
        printOpenSSLError();
        EVP_MD_CTX_free(mdctx);
        return;
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
    else
    {
        signature.resize(sigLen);
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

/*void verifySignature()
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

    // NOTE: This legacy function did not have access to the actual model bytes nor a real signature.
    // Keep it as a no-op warning to avoid giving a false sense of security.
    std::cerr << "Warning: verifySignature() has no model/signature input. "
              << "Use verifyGlobalModelSignatureWithKey(model, signature, publicKey) instead." << std::endl;

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(publicKey);
}*/

/*void simulateKeyFetch()
{
    // Simulate the key fetch from GMC
    std::this_thread::sleep_for(std::chrono::milliseconds(550));
}*/

/*
// Function to simulate the decryption of a 1.3 MB file
void simulateDecryption()
{

    // Simulate a computational delay (for instance, assume decryption takes 10 milliseconds)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
*/

void federatedAvg(const string &pathToFiles, const string outputPath, int numFiles)
{

    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

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

    // Read the weights and biases from each file and add them to the sum
    for (int i = 0; i < numFiles; i++)
    {

        // additional overhead
        //simulateDecryption();

        // Read the weights and biases from the file
        MatrixXd w1 = MatrixXd::Zero(200, 784);
        MatrixXd b1 = MatrixXd::Zero(200, 1);
        MatrixXd w2 = MatrixXd::Zero(50, 200);
        MatrixXd b2 = MatrixXd::Zero(50, 1);
        MatrixXd w3 = MatrixXd::Zero(10, 50);
        MatrixXd b3 = MatrixXd::Zero(10, 1);

        streamoff position = 0;
        const string workerModelPath = pathToFiles + std::to_string(i) + ".bin";
        position = read(&w1, position, workerModelPath);
        position = read(&b1, position, workerModelPath);
        position = read(&w2, position, workerModelPath);
        position = read(&b2, position, workerModelPath);
        position = read(&w3, position, workerModelPath);
        read(&b3, position, workerModelPath);

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

    // Sign the global model FILE bytes (outputPath) and write signature beside it
    std::ifstream gmFile(outputPath, std::ios::binary);
    if (!gmFile.is_open())
    {
        cerr << "Error: Could not open saved global model for signing: " << outputPath << endl;
        EVP_PKEY_free(privateKey);
        EVP_cleanup();
        ERR_free_strings();
        return;
    }

    std::string gmBytes((std::istreambuf_iterator<char>(gmFile)), std::istreambuf_iterator<char>());
    gmFile.close();

    std::string signatureBytes;
    if (!signBytesWithKey(privateKey, gmBytes.data(), gmBytes.size(), signatureBytes))
    {
        cerr << "Error: Failed to sign global model bytes" << endl;
        EVP_PKEY_free(privateKey);
        EVP_cleanup();
        ERR_free_strings();
        return;
    }

    const std::string sigPath = outputPath + ".sig";
    std::ofstream sigFile(sigPath, std::ios::binary);
    if (!sigFile.is_open())
    {
        cerr << "Error: Could not open signature output file: " << sigPath << endl;
        EVP_PKEY_free(privateKey);
        EVP_cleanup();
        ERR_free_strings();
        return;
    }
    sigFile.write(signatureBytes.data(), (std::streamsize)signatureBytes.size());
    sigFile.close();

    cout << "Global model signature written: " << sigPath << " (bytes=" << signatureBytes.size() << ")\n";

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
