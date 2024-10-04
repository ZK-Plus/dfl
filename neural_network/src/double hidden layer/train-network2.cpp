#include <fstream>
#include <iostream>
#include <chrono>
#include <Eigen/Dense>
#include <signal.h>
#include "../functions.h"
#include "train-network2.h"
#include "network2.h"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <thread>

using namespace std;
using Eigen::MatrixXd;
const int AES_KEY_LENGTH = 256;

// Initialize weights and biases to a random value between -0.5 and 0.5
weights_and_biases wab;

void save_weights_and_biases(const string &file_path)
{
    cout << "Saving weights and biases to file...\n";
    streamoff write_position = 0;
    write_position = save(wab.W1, write_position, file_path);
    write_position = save(wab.B1, write_position, file_path);
    write_position = save(wab.W2, write_position, file_path);
    write_position = save(wab.B2, write_position, file_path);
    write_position = save(wab.W3, write_position, file_path);
    save(wab.B3, write_position, file_path);

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

void read_weights_and_biases(const string &path)
{
    cout << "Reading weights and biases from file...\n";

    // check if the file exists
    ifstream file(path);
    if (file.good())
    {
        file.close();
        streamoff read_position = 0;
        read_position = read(&wab.W1, read_position, path);
        read_position = read(&wab.B1, read_position, path);
        read_position = read(&wab.W2, read_position, path);
        read_position = read(&wab.B2, read_position, path);
        read_position = read(&wab.W3, read_position, path);
        read(&wab.B3, read_position, path);
    }
}

// ensures that the program saves the weights and biases to a file when the user presses Ctrl+C
void signal_callback_handler(int signum)
{
    // Optionally save weights and biases to file
    if (SAVE_WEIGHTS_AND_BIASES)
    {
        save_weights_and_biases("./data/backup.bin");
    }

    exit(signum);
}

void saveRandomWBasFile()
{
    weights_and_biases random_start_model;
    random_start_model.W1 = MatrixXd::Random(L1_SIZE, 784) / 2;
    random_start_model.B1 = MatrixXd::Random(L1_SIZE, 1) / 2;
    random_start_model.W2 = MatrixXd::Random(L2_SIZE, L1_SIZE) / 2;
    random_start_model.B2 = MatrixXd::Random(L2_SIZE, 1) / 2;
    random_start_model.W3 = MatrixXd::Random(10, L2_SIZE) / 2;
    random_start_model.B3 = MatrixXd::Random(10, 1) / 2;

    cout << "Saving weights and biases to file...\n";
    streamoff write_position = 0;
    write_position = save(random_start_model.W1, write_position, "./data/random_start.bin");
    write_position = save(random_start_model.B1, write_position, "./data/random_start.bin");
    write_position = save(random_start_model.W2, write_position, "./data/random_start.bin");
    write_position = save(random_start_model.B2, write_position, "./data/random_start.bin");
    write_position = save(random_start_model.W3, write_position, "./data/random_start.bin");
    save(random_start_model.B3, write_position, "./data/random_start.bin");
    cout << "Weights and biases saved to file\n";
}

// Function to load the private key from a PEM file
EVP_PKEY *loadPrivateKeyLM(const char *privateKeyPath)
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
EVP_PKEY *loadPublicKeyL(const char *publicKeyPath)
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
void printOpenSSLErrorLocal()
{
    unsigned long errCode = ERR_get_error();
    char errMsg[256];
    ERR_error_string_n(errCode, errMsg, sizeof(errMsg));
    cerr << "OpenSSL Error: " << errMsg << endl;
}

// Function to create a digital signature of the data
void signLocalModel(const MatrixXd &model, EVP_PKEY *privateKey, string &signature)
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
        printOpenSSLErrorLocal();
        EVP_MD_CTX_free(mdctx);
        return;
    }

    // Update the signing context with the model data
    for (int i = 0; i < model.size(); ++i)
    {
        if (EVP_DigestSignUpdate(mdctx, &model(i), sizeof(model(i))) != 1)
        {
            cerr << "Error: EVP_DigestSignUpdate failed" << endl;
            printOpenSSLErrorLocal();
            EVP_MD_CTX_free(mdctx);
            return;
        }
    }

    // Finalize the signature
    size_t sigLen = 0;
    if (EVP_DigestSignFinal(mdctx, NULL, &sigLen) != 1)
    {
        cerr << "Error: EVP_DigestSignFinal (getting signature size) failed" << endl;
        printOpenSSLErrorLocal();
        EVP_MD_CTX_free(mdctx);
        return;
    }

    signature.resize(sigLen);
    if (EVP_DigestSignFinal(mdctx, (unsigned char *)signature.data(), &sigLen) != 1)
    {
        cerr << "Error: EVP_DigestSignFinal (signing) failed" << endl;
        printOpenSSLErrorLocal();
    }

    EVP_MD_CTX_free(mdctx);
    cout << "Global model signed successfully." << endl;
}

// Function to encrypt the global model using AES encryption
void encryptLocalModel(const MatrixXd &model, string &encryptedData, const unsigned char *key, const unsigned char *iv)
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
        printOpenSSLErrorLocal();
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
            printOpenSSLErrorLocal();
            EVP_CIPHER_CTX_free(ctx);
            return;
        }
        encryptedData.append(reinterpret_cast<char *>(outbuf), len);
    }

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, outbuf, &len) != 1)
    {
        cerr << "Error: EVP_EncryptFinal_ex failed" << endl;
        printOpenSSLErrorLocal();
    }
    encryptedData.append(reinterpret_cast<char *>(outbuf), len);

    EVP_CIPHER_CTX_free(ctx);
    cout << "Global model encrypted successfully." << endl;
}

void verifySignatureGM()
{
    cout << "Verifying signature..." << endl;
    // Load the public key
    EVP_PKEY *publicKey = loadPublicKeyL("public_key.pem");
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
        printOpenSSLErrorLocal();
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(publicKey);
        return;
    }

    // Simulate data processing (in a real case, you would hash the data)
    string data = "Simulated data";
    if (EVP_DigestVerifyUpdate(mdctx, data.c_str(), data.length()) != 1)
    {
        cerr << "Error: DigestVerifyUpdate failed" << endl;
        printOpenSSLErrorLocal();
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

void simulateKeyFetchLM()
{
    // Simulate the key fetch from GMC
    std::this_thread::sleep_for(std::chrono::milliseconds(550));
}

int train_network(const string &wb_in, const string &wb_out, const string &image_path, const string &label_path, const int epoch_amount)
{
    // Register signal handler
    signal(SIGINT, signal_callback_handler);

    // Randomize the starting seed
    srand((unsigned int)time(nullptr));

    // overhead through signature verification
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    simulateKeyFetchLM();
    verifySignatureGM();

    // Simulate a private key for signing
    EVP_PKEY *privateKey = loadPrivateKeyLM("private_key.pem");
    if (!privateKey)
    {
        cerr << "Error: Could not load private key." << endl;
        return 1;
    }

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
        read_weights_and_biases(wb_in);
    }

    // For each epoch, perform gradient descent and update weights and biases
    for (int epoch = 1; epoch <= epoch_amount; epoch++)
    {
        // Get start time
        auto start = chrono::high_resolution_clock::now();

        // Store number of correct predictions
        int count = gradient_descent(wab, LEARNING_RATE, epoch, image_path, label_path);

        // Get end time
        auto end = chrono::high_resolution_clock::now();

        // Calculate duration of time passed
        double duration = (double)chrono::duration_cast<chrono::microseconds>(end - start).count() / 1000000.0;

        // Calculate remaining time
        int seconds = (int)duration * (epoch_amount - epoch);
        int minutes = seconds / 60;
        int hours = minutes / 60;
        minutes %= 60;
        seconds %= 60;

        // Print the results of the epoch
        // cout << "Epoch: " << epoch << "/" << NUM_EPOCHS << "\n";
        // cout << "Accuracy: " << count << "/" << NUM_TRAIN_IMAGES << "\n";
        // cout << "Time taken: " << duration << " seconds \n";
        // cout << "Estimated time remaining: ";
        // printf("%02d:%02d:%02d\n", hours, minutes, seconds);
        // cout << "\n";
    }

    cout << "Finished training!\n";

    // after epoch, sign and encrypt the model
    // Encrypt the local model
    string encryptedModel;
    unsigned char aesKey[AES_KEY_LENGTH / 8]; // AES-256 key
    unsigned char iv[16];                     // Initialization vector
    RAND_bytes(aesKey, sizeof(aesKey));       // Generate random AES key
    RAND_bytes(iv, sizeof(iv));               // Generate random IV
    encryptLocalModel(wab.W1, encryptedModel, aesKey, iv);
    encryptLocalModel(wab.B1, encryptedModel, aesKey, iv);
    encryptLocalModel(wab.W2, encryptedModel, aesKey, iv);
    encryptLocalModel(wab.B2, encryptedModel, aesKey, iv);
    encryptLocalModel(wab.W3, encryptedModel, aesKey, iv);
    encryptLocalModel(wab.B3, encryptedModel, aesKey, iv);

    // Sign the global model
    string signature;
    signLocalModel(wab.W1, privateKey, signature);

    EVP_PKEY_free(privateKey);
    EVP_cleanup();
    ERR_free_strings();

    // Optionally save weights and biases to file
    if (SAVE_WEIGHTS_AND_BIASES)
    {
        // TODO: adapt to dynamic param
        save_weights_and_biases(wb_out);
        // save_weight_and_biases_as_csv();
    }

    return 0;
}
