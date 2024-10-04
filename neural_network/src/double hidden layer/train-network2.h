//
// Created by Johann Hartmann on 19.03.24.
//

#ifndef NEURAL_NETWORK_TRAIN_NETWORK2_H
#define NEURAL_NETWORK_TRAIN_NETWORK2_H

#include <string>

// Declare the function so other files know it exists and can call it
int train_network(const std::string &wb_in, const std::string &wb_out, const std::string &image_path, const std::string &label_path, const int epoch_amount);

void saveRandomWBasFile();

#endif // NEURAL_NETWORK_TRAIN_NETWORK2_H
