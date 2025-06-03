// File: src/risk_engine.cpp
#include "risk_engine.h"
#include "config_loader.h"
#include "utils/logger.h"
#include <iostream>

using namespace rms;

RiskEngine::RiskEngine() = default;

RiskEngine::~RiskEngine() {
    stop();
}

bool RiskEngine::initialize(const std::string &config_path) {
    if (!ConfigLoader::loadConfig(config_path)) {
        return false;
    }
    // Initialize Messaging with callbacks bound to this instance
    bool ok = messaging_.initialize(
        [this](const Order &order) { this->onOrderReceived(order); },
        [this](const TradeExecution &trade) { this->onTradeReceived(trade); }
    );
    if (!ok) {
        std::cerr << "[RiskEngine] Failed to initialize Messaging\n";
        return false;
    }
    return true;
}

void RiskEngine::start() {
    running_ = true;

    // Launch shard threads
    for (int i = 0; i < NUM_SHARDS; ++i) {
        shard_threads_.emplace_back(&RiskEngine::runShard, this, i);
    }

    utils::logInfo("[RiskEngine] Shard threads started");
    utils::logInfo("[RiskEngine] Listening for messages via Aeron");
    // Messaging already spun up its listener thread in initialize()
}

void RiskEngine::stop() {
    running_ = false;

    // Join shard threads
    for (auto &t : shard_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    shard_threads_.clear();

    // Shut down messaging
    messaging_.shutdown();

    utils::logInfo("[RiskEngine] Stopped all threads and messaging");
}

void RiskEngine::runShard(int shard_id) {
    utils::logInfo("[RiskEngine] runShard started for shard " + std::to_string(shard_id));
    while (running_) {
        // In a full implementation, you would process any queued orders/trades
        // or perform periodic risk calculations here. For now, we sleep.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    utils::logInfo("[RiskEngine] runShard exiting for shard " + std::to_string(shard_id));
}

void RiskEngine::onOrderReceived(const Order &order) {
    utils::logInfo("[RiskEngine] Received order received");
    // Determine which shard this order belongs to
    int shard = static_cast<int>(order.account_id % NUM_SHARDS);

    // 1) Pre-trade checks
    bool passQty   = pretrade_checks_[shard].checkMaxOrderQty(order);
    bool passPos   = pretrade_checks_[shard].checkPositionLimit(order);
    // We don't have a reference price here; assume checkPriceBand not used for now

    if (!passQty) {
        utils::logError("[RiskEngine] Order rejected: max qty exceeded for account " +
                         std::to_string(order.account_id));
        return;
    }
    if (!passPos) {
        utils::logError("[RiskEngine] Order rejected: position limit for account " +
                         std::to_string(order.account_id));
        return;
    }

    // If passed, send the order to the matching engine (not implemented here).
    utils::logInfo("[RiskEngine] Order accepted: account " +
                   std::to_string(order.account_id) + ", qty " + std::to_string(order.quantity));
}

void RiskEngine::onTradeReceived(const TradeExecution &trade) {
    // Determine shard
    int shard = static_cast<int>(trade.account_id % NUM_SHARDS);

    // Post-trade update (positions, PnL, margin)
    posttrade_controls_[shard].onTrade(trade);

    // After updating positions, send a trade confirmation back if needed
    // (In this example, we simply log it)
    utils::logInfo("[RiskEngine] Trade received: account " +
                   std::to_string(trade.account_id) + ", qty " + std::to_string(trade.quantity) +
                   ", price " + std::to_string(trade.price));
}
