//
// Created by muhammad-abdullah on 5/27/25.
//
// File: src/utils/logger.cpp
#include "utils/logger.h"
#include <iostream>

void rms::utils::logInfo(const std::string &msg) {
    std::cout << "[INFO] " << msg << std::endl;
}

void rms::utils::logError(const std::string &msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}