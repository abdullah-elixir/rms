#pragma once

#include <yaml-cpp/yaml.h>
#include <string>
#include <optional>
#include <memory>
#include <mutex>

namespace rms {

class Config {
public:
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    bool loadFromFile(const std::string& config_file) {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            config_ = YAML::LoadFile(config_file);
            return true;
        } catch (const YAML::Exception& e) {
            return false;
        }
    }

    // Database configuration
    std::string getDbPath() const {
        return config_["database"]["path"].as<std::string>();
    }

    int getDbWriteBufferSize() const {
        return config_["database"]["write_buffer_size"].as<int>();
    }

    // Sharding configuration
    int getShardCount() const {
        return config_["sharding"]["count"].as<int>();
    }

    // Risk limits
    double getDefaultMaxLeverage() const {
        return config_["risk_limits"]["default_max_leverage"].as<double>();
    }

    double getDefaultMaxDrawdown() const {
        return config_["risk_limits"]["default_max_drawdown"].as<double>();
    }

    // Performance tuning
    int getMaxConcurrentOrders() const {
        return config_["performance"]["max_concurrent_orders"].as<int>();
    }

    int getOrderQueueSize() const {
        return config_["performance"]["order_queue_size"].as<int>();
    }

    // Logging configuration
    std::string getLogLevel() const {
        return config_["logging"]["level"].as<std::string>();
    }

    std::string getLogFile() const {
        return config_["logging"]["file"].as<std::string>();
    }

    // Metrics configuration
    int getMetricsPort() const {
        return config_["metrics"]["port"].as<int>();
    }

    std::string getMetricsEndpoint() const {
        return config_["metrics"]["endpoint"].as<std::string>();
    }

private:
    Config() = default;
    YAML::Node config_;
    mutable std::mutex mutex_;
};

} // namespace rms 