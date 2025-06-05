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
    bool ok = messaging_.initialize();
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
    std::cout << "[RiskEngine]|"<<shard_id<< " Running shard: " << shard_id << std::endl;
    aeron::concurrent::BackoffIdleStrategy idle_strategy(100, 1000);
    auto& queue = messaging_.getQueue()[shard_id];
    bool processed = false;
    while (running_) {
        for (int i = 0; i < MAX_FRAGMENT_BATCH_SIZE; ++i) {
            auto msg = queue.dequeue();
            if (msg.has_value()) {
                processed = true;
                std::cout<<"RiskEngine|"<<shard_id<<" dequeing completed on shard: "<<shard_id<<std::endl;
                auto variant = msg.value();
                if (std::holds_alternative<Order>(variant)) {
                    onOrderReceived(std::get<Order>(variant));
                }
                else if (std::holds_alternative<TradeExecution>(variant)) {
                    onTradeReceived(std::get<TradeExecution>(variant));
                }
                else {
                    std::cout<<"[RiskEngine]|"<<shard_id<< " Unknown or malformed message received"<<std::endl;
                }
            }
            else
                break;
        }
        if (!processed) {
            idle_strategy.idle();
            //std::this_thread::sleep_for(std::chrono::microseconds(10));
            //std::this_thread::yield(); // or sleep_for(100us) for cooler CPU
        }
        processed = false;
    }
    std::cout<<"[RiskEngine]|" << shard_id<<" runShard exiting for shard "<<std::to_string(shard_id)<<std::endl;
}

void RiskEngine::onOrderReceived(const Order &order) {
    utils::logInfo("[RiskEngine] Received order");
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
    std::cout<<"-----FOOTER-----"<<std::endl;
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
