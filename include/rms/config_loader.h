//
// Created by muhammad-abdullah on 5/27/25.
//

// File: include/rms/config_loader.hpp
#pragma once
#include <string>

namespace rms {
    struct ConfigLoader {
        static bool loadConfig(const std::string &filepath);
    };
}