//
// Created by muhammad-abdullah on 5/27/25.
//
// File: src/risk_engine.cpp
#include "risk_engine.h"
#include "config_loader.h"
#include "utils/logger.h"
#include <iostream>

#include "data_types.h"

rms::RiskEngine::RiskEngine() {}

rms::RiskEngine::~RiskEngine() {
    stop();
}

bool rms::RiskEngine::initialize(const std::string &config_path) {
    if (!ConfigLoader::loadConfig(config_path)) return false;
    return true;
}

void rms::RiskEngine::start() {
    running_ = true;
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shard_threads_.emplace_back(&RiskEngine::runShard, this, i);
    }
    utils::logInfo("RiskEngine started");
}

void rms::RiskEngine::stop() {
    running_ = false;
    for (auto &t : shard_threads_) {
        if (t.joinable()) t.join();
    }
    utils::logInfo("RiskEngine stopped");
}

void rms::RiskEngine::runShard(int shard_id) {
    while (running_) {
        // Simplified: sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
