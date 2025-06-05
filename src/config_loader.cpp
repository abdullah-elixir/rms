//
// Created by muhammad-abdullah on 5/27/25.
//
// File: src/config_loader.cpp
#include "config_loader.h"
#include <yaml-cpp/yaml.h>
#include "data_types.h"
#include <iostream>

bool rms::ConfigLoader::loadConfig(const std::string &filepath) {
    try {
        YAML::Node config = YAML::LoadFile(filepath);

        std::cout << "Loaded config from " << filepath << std::endl;
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
        return false;
    }
}
