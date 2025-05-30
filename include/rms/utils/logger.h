//
// Created by muhammad-abdullah on 5/27/25.
//

// File: include/rms/utils/logger.hpp
#pragma once
#include <string>

namespace rms::utils {
    void logInfo(const std::string &msg);
    void logError(const std::string &msg);
}