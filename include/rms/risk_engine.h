//
// Created by muhammad-abdullah on 5/27/25.
//

// File: include/rms/risk_engine.hpp
#pragma once
#include <thread>
#include <vector>

namespace rms {
    class RiskEngine {
    public:
        RiskEngine();
        ~RiskEngine();
        bool initialize(const std::string &config_path);
        void start();
        void stop();
    private:
        void runShard(int shard_id);
        std::vector<std::thread> shard_threads_;
        bool running_ = false;
    };
}