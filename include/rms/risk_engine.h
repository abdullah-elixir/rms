// File: include/rms/risk_engine.hpp
#pragma once

#include <thread>
#include <vector>
#include <concurrent/BackOffIdleStrategy.h>
#include "logger.h"
#include "loggerwrapper.h"
#include "data_types.h"
#include "messaging.h"
#include "pretrade_checks.h"
#include "posttrade_controls.h"

namespace rms
{

    class RiskEngine
    {
    public:
        RiskEngine();
        ~RiskEngine();

        /// Load configuration and initialize modules.
        bool initialize(const std::string &config_path);

        /// Start shard threads and messaging.
        void start();

        /// Stop shards and messaging.
        void stop();

        /// Save current state to persistent storage
        bool saveState();

        /// Load state from persistent storage
        bool loadState();
        // logger wrapper for shard logging
        std::unique_ptr<LoggerWrapper> logger_wrapper_;

    private:
        void runShard(int shard_id);

        // Callback invoked by Messaging when a new Order arrives
        void onOrderReceived(const Order &order, int shard_id);

        // Callback invoked by Messaging when a TradeExecution arrives
        void onTradeReceived(const TradeExecution &trade, int shard_id);

        std::vector<std::thread> shard_threads_;
        bool running_ = false;

        // One PreTradeChecks and PostTradeControls per shard
        PreTradeChecks pretrade_checks_[NUM_SHARDS];
        PostTradeControls posttrade_controls_[NUM_SHARDS];

        // Messaging instance (wraps Aeron pub/sub)
        Messaging messaging_;

        // base logger
        Logger blogger_;
        // Persistence manager
        // PersistenceManager persistence_;
    };

} // namespace rms
