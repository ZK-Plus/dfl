#include <fstream>
#include <iostream>
#include <chrono>
#include <Eigen/Dense>
#include <signal.h>
#include "../functions.h"
#include "train-network2.h"
#include "network2.h"
#include <openssl/evp.h>
#include <openssl/x509.h>   // d2i_PUBKEY
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <thread>
#include <cctype>
#include <string>
#include <vector>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <cstring>
#include <cstdint>

using namespace std;
using Eigen::MatrixXd;
const int AES_KEY_LENGTH = 256;

static bool hexToBytes(const std::string &hexIn, std::vector<unsigned char> &out);
static EVP_PKEY *loadPublicKeyFromDerHex(const std::string &derHex);
static bool rsaOaepSha256Encrypt(EVP_PKEY *publicKey,
                                const unsigned char *in,
                                size_t inLen,
                                std::string &out);

void printOpenSSLErrorLocal();

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

static bool encryptAes256Cbc(const std::string &plaintext,
                            std::string &ciphertext,
                            const unsigned char *key,
                            const unsigned char *iv)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        cerr << "Error: Failed to create EVP_CIPHER_CTX" << endl;
        return false;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
    {
        cerr << "Error: EVP_EncryptInit_ex failed" << endl;
        printOpenSSLErrorLocal();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    ciphertext.clear();
    ciphertext.resize(plaintext.size() + AES_BLOCK_SIZE);

    int outLen1 = 0;
    if (EVP_EncryptUpdate(ctx,
                          reinterpret_cast<unsigned char *>(ciphertext.data()),
                          &outLen1,
                          reinterpret_cast<const unsigned char *>(plaintext.data()),
                          (int)plaintext.size()) != 1)
    {
        cerr << "Error: EVP_EncryptUpdate failed" << endl;
        printOpenSSLErrorLocal();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int outLen2 = 0;
    if (EVP_EncryptFinal_ex(ctx,
                           reinterpret_cast<unsigned char *>(ciphertext.data()) + outLen1,
                           &outLen2) != 1)
    {
        cerr << "Error: EVP_EncryptFinal_ex failed" << endl;
        printOpenSSLErrorLocal();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    ciphertext.resize((size_t)outLen1 + (size_t)outLen2);
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

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
    cout << "Local model signed successfully." << endl;
}

// Function to encrypt the local model using AES encryption
void encryptLocalModel(const MatrixXd &model, string &encryptedData, const unsigned char *key, const unsigned char *iv)
{
    cout << "Encrypting local model..." << endl;
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
    cout << "Local model encrypted successfully." << endl;
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

int train_network(const string &wb_in, const string &wb_out, const string &image_path, const string &label_path, const int epoch_amount, const string &current_aggregator_public_rsa_key)
{
    // Register signal handler
    signal(SIGINT, signal_callback_handler);

    // Randomize the starting seed
    srand((unsigned int)time(nullptr));

    // overhead through signature verification
    //OpenSSL_add_all_algorithms();
    //ERR_load_crypto_strings();

    //simulateKeyFetchLM();
    //verifySignatureGM();

    // Simulate a private key for signing
    EVP_PKEY *privateKey = loadPrivateKeyLM("private_key.pem");
    if (!privateKey)
    {
        cerr << "Error: Could not load private key." << endl;
        return 1;
    }

    // Convert the public key to DER format for sending
    std::string senderPubDer;
    {
        int len = i2d_PUBKEY(privateKey, nullptr);
        if (len <= 0) {
            cerr << "Error: i2d_PUBKEY(size) failed" << endl;
            printOpenSSLErrorLocal();
            EVP_PKEY_free(privateKey);
            return 1;
        }

        senderPubDer.resize((size_t)len);
        unsigned char *p = reinterpret_cast<unsigned char *>(senderPubDer.data());
        if (i2d_PUBKEY(privateKey, &p) <= 0) {
            cerr << "Error: i2d_PUBKEY failed" << endl;
            printOpenSSLErrorLocal();
            EVP_PKEY_free(privateKey);
            return 1;
        }
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
        cout << "Epoch: " << epoch << "/" << NUM_EPOCHS << "\n";
        cout << "Accuracy: " << count << "/" << NUM_TRAIN_IMAGES << "\n";
        cout << "Time taken: " << duration << " seconds \n";
        cout << "Estimated time remaining: ";
        printf("%02d:%02d:%02d\n", hours, minutes, seconds);
        cout << "\n";
    }

    cout << "Finished training!\n";

    if (current_aggregator_public_rsa_key.empty())
    {
        cout << "Skipping encryption: current_aggregator_public_rsa_key not provided.\n";
        EVP_PKEY_free(privateKey);
        if (SAVE_WEIGHTS_AND_BIASES)
        {
            save_weights_and_biases(wb_out);
        }
        return 0;
    }

    // after epoch, sign and encrypt the model
    // Encrypt the local model (all 6 matrices together, in the same order/format as save())
    std::string plaintextModel;
    plaintextModel.reserve((wab.W1.size() + wab.B1.size() + wab.W2.size() + wab.B2.size() + wab.W3.size() + wab.B3.size()) * sizeof(double));
    appendMatrixAsSaveFormat(wab.W1, plaintextModel);
    appendMatrixAsSaveFormat(wab.B1, plaintextModel);
    appendMatrixAsSaveFormat(wab.W2, plaintextModel);
    appendMatrixAsSaveFormat(wab.B2, plaintextModel);
    appendMatrixAsSaveFormat(wab.W3, plaintextModel);
    appendMatrixAsSaveFormat(wab.B3, plaintextModel);

    cout << "Model serialization OK (plaintext bytes=" << plaintextModel.size() << ")\n";

    unsigned char aesKey[AES_KEY_LENGTH / 8]; // AES-256 key
    unsigned char iv[16];                     // Initialization vector
    if (RAND_bytes(aesKey, sizeof(aesKey)) != 1 || RAND_bytes(iv, sizeof(iv)) != 1)
    {
        cerr << "Error: RAND_bytes failed" << endl;
        printOpenSSLErrorLocal();
        EVP_PKEY_free(privateKey);
        return 1;
    }

    std::string encryptedModel;
    if (!encryptAes256Cbc(plaintextModel, encryptedModel, aesKey, iv))
    {
        cerr << "Error: Model encryption failed" << endl;
        EVP_PKEY_free(privateKey);
        return 1;
    }

    cout << "AES-256-CBC encryption OK (ciphertext bytes=" << encryptedModel.size() << ")\n";

    // RSA-wrap AES key + IV using aggregator public key (DER-hex, starts with 0x)
    EVP_PKEY *aggPubKey = loadPublicKeyFromDerHex(current_aggregator_public_rsa_key);
    if (!aggPubKey)
    {
        cerr << "Error: Could not parse aggregator public key (DER hex)" << endl;
        EVP_PKEY_free(privateKey);
        return 1;
    }

    cout << "Aggregator public key parsed OK (DER-hex)\n";

    unsigned char keyIv[sizeof(aesKey) + sizeof(iv)];
    memcpy(keyIv, aesKey, sizeof(aesKey));
    memcpy(keyIv + sizeof(aesKey), iv, sizeof(iv));

    std::string wrappedKeyIv;
    if (!rsaOaepSha256Encrypt(aggPubKey, keyIv, sizeof(keyIv), wrappedKeyIv))
    {
        cerr << "Error: RSA OAEP encrypt(key||iv) failed" << endl;
        EVP_PKEY_free(aggPubKey);
        EVP_PKEY_free(privateKey);
        return 1;
    }
    EVP_PKEY_free(aggPubKey);

    cout << "RSA-OAEP-SHA256 wrap(key||iv) OK (wrapped bytes=" << wrappedKeyIv.size() << ")\n";

    // Write .enc as a single binary package:
    // [u32 wrappedLen][wrappedKeyIv][u32 ciphertextLen][ciphertext]
    const std::string encrypted_out = wb_out + ".enc";
    std::ofstream outEnc(encrypted_out, std::ios::binary);
    if (!outEnc.is_open())
    {
        cerr << "Error: Could not open encrypted output file: " << encrypted_out << endl;
        EVP_PKEY_free(privateKey);
        return 1;
    }

    const uint32_t senderPubLen = (uint32_t)senderPubDer.size();
    const uint32_t wrappedLen   = (uint32_t)wrappedKeyIv.size();
    const uint32_t ctLen        = (uint32_t)encryptedModel.size();

    outEnc.write(reinterpret_cast<const char *>(&senderPubLen), sizeof(senderPubLen));
    outEnc.write(senderPubDer.data(), (std::streamsize)senderPubDer.size());

    outEnc.write(reinterpret_cast<const char *>(&wrappedLen), sizeof(wrappedLen));
    outEnc.write(wrappedKeyIv.data(), (std::streamsize)wrappedKeyIv.size());

    outEnc.write(reinterpret_cast<const char *>(&ctLen), sizeof(ctLen));
    outEnc.write(encryptedModel.data(), (std::streamsize)encryptedModel.size());
    outEnc.close();

        cout << "Encrypted package written: " << encrypted_out
            << " (senderPubDer=" << senderPubLen
            << ", wrapped=" << wrappedLen
            << ", ct=" << ctLen << ")\n";

    //// Sign the global model
    //string signature;
    //signLocalModel(wab.W1, privateKey, signature);

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

static bool rsaOaepSha256Encrypt(EVP_PKEY *publicKey,
                                const unsigned char *in,
                                size_t inLen,
                                std::string &out)
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(publicKey, nullptr);
    if (!ctx) { printOpenSSLErrorLocal(); return false; }

    if (EVP_PKEY_encrypt_init(ctx) <= 0 ||
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0 ||
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) <= 0)
    {
        printOpenSSLErrorLocal();
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    size_t outLen = 0;
    if (EVP_PKEY_encrypt(ctx, nullptr, &outLen, in, inLen) <= 0)
    {
        printOpenSSLErrorLocal();
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    out.resize(outLen);
    if (EVP_PKEY_encrypt(ctx,
                         reinterpret_cast<unsigned char *>(out.data()),
                         &outLen,
                         in,
                         inLen) <= 0)
    {
        printOpenSSLErrorLocal();
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    out.resize(outLen);
    EVP_PKEY_CTX_free(ctx);
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

    if (cleaned.size() % 2 != 0)
        return false;

    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
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
        printOpenSSLErrorLocal();
        return nullptr;
    }
    return pub;
}
