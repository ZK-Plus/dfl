#ifndef DIGIT_RECOGNITION_FUNCTIONS_H
#define DIGIT_RECOGNITION_FUNCTIONS_H

#define TRAIN_LABELS_FILE_PATH R"(./data/iid/train-labels.idx1-ubyte)"
#define TRAIN_IMAGES_FILE_PATH R"(./data/iid/train-images.idx3-ubyte)"
#define TEST_LABELS_FILE_PATH R"(./data/t10k-labels.idx1-ubyte)"
#define TEST_IMAGES_FILE_PATH R"(./data/t10k-images.idx3-ubyte)"

#define LABEL_START 8
#define IMAGE_START 16
#define BATCH_SIZE 32

#define NUM_TRAIN_IMAGES 10000
#define NUM_BATCHES (NUM_TRAIN_IMAGES / BATCH_SIZE)
#define NUM_TEST_IMAGES 10000
#define LEARNING_RATE 0.1
#define NUM_EPOCHS 50 // standard value = 500
#define ACTIVATION_FUNCTION TANH

#define SAVE_WEIGHTS_AND_BIASES true
#define PRINT_LABELS_AND_IMAGES false

#include <Eigen/Dense>
#include <string>

enum Activation
{
    TANH = 0,
    RELU = 1,
    LEAKY_RELU = 2
};

void federatedAvg(const std::string &pathToFiles, const std::string outputPath, int numFiles);

std::streamoff save(const Eigen::MatrixXd &X, std::streamoff position, const std::string &path);

std::streamoff read(Eigen::MatrixXd *X, std::streamoff position, const std::string &path);

Eigen::MatrixXd get_labels(int offset, int size, const std::string &path);

Eigen::MatrixXd get_images(int offset, int size, const std::string &path);

Eigen::MatrixXd get_label_batch(const int offsets[], int index, int size, const std::string &path);

Eigen::MatrixXd get_image_batch(const int offsets[], int index, int size, const std::string &path);

void print_batch(const Eigen::MatrixXd &X, const Eigen::MatrixXd &Y, int size);

Eigen::MatrixXd softmax(const Eigen::MatrixXd &Z);

Eigen::MatrixXd deriv_tanh(const Eigen::MatrixXd &Z);

double ReLU(double x);

double deriv_ReLU(double x);

double leaky_ReLU(double x);

double deriv_leaky_ReLU(double x);

Eigen::MatrixXd get_predictions(const Eigen::MatrixXd &AL, int size);

int get_num_correct(const Eigen::MatrixXd &P, const Eigen::MatrixXd &Y, int size);

// --- Signature helpers (sign/verify only) ---
// Signs exactly the bytes written by save() for the provided matrix.
// Returns true on success and fills signatureOut with raw signature bytes.
bool signModelToSignature(const Eigen::MatrixXd &model,
                          const std::string &privateKeyPemPath,
                          std::string &signatureOut);

// Verifies signature over the bytes written by save() for the provided matrix.
// signature is raw signature bytes (not hex/base64).
bool verifyModelSignature(const Eigen::MatrixXd &model,
                          const std::string &signature,
                          const std::string &publicKeyPemPath);

// Verifies a model file against its signature file.
// Expects modelPath to be the binary model (e.g. outputPath) and sigPath to be raw signature bytes (e.g. outputPath + ".sig").
bool verifyModelFileSignature(const std::string &modelPath,
                              const std::string &sigPath,
                              const std::string &publicKeyDerHex);

// Signs arbitrary file bytes and writes raw signature bytes to sigOutPath.
// privateKeyPemPath should point to a PEM-encoded private key (e.g. private_key.pem).
bool signFileToSignature(const std::string &filePath,
                         const std::string &privateKeyPemPath,
                         const std::string &sigOutPath);

#endif
