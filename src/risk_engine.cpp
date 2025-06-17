// File: src/risk_engine.cpp
#include "risk_engine.h"
#include "config_loader.h"
#include "logger.h"
#include <iostream>

using namespace rms;

RiskEngine::RiskEngine() : blogger_("../log/risk_engine/risk_engine.log") {
};

RiskEngine::~RiskEngine() {
    stop();
}

bool RiskEngine::initialize(const std::string &config_path) {
    if (!ConfigLoader::loadConfig(config_path)) {
        return false;
    }
    std::cout << "initializing the logger: " << config_path << std::endl;
    //initializing logger
    logger_wrapper_ = std::make_unique<LoggerWrapper>(4, "../log/risk_engine/risk_engine"); 
    // Initialize Messaging with callbacks bound to this instance
    bool ok = messaging_.initialize(logger_wrapper_);
    if (!ok) {
        blogger_.error("[RiskEngine] Failed to initialize Messaging");
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

    blogger_.debug("[RiskEngine] Shard threads started");
    blogger_.debug("[RiskEngine] Listening for messages via Aeron");
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

    blogger_.debug("[RiskEngine] Stopped all threads and messaging");
}

void RiskEngine::runShard(int shard_id) {
    logger_wrapper_->debug(shard_id, "[RiskEngine] Running shard");
    aeron::concurrent::BackoffIdleStrategy idle_strategy(100, 1000);
    auto& queue = messaging_.getQueue()[shard_id];
    bool processed = false;
    while (running_) {
        for (int i = 0; i < MAX_FRAGMENT_BATCH_SIZE; ++i) {
            auto msg = queue.dequeue();
            if (msg.has_value()) {
                processed = true;
                logger_wrapper_->debug(shard_id, "RiskEngine dequeing completed on shard");
                auto variant = msg.value();
                if (std::holds_alternative<Order>(variant)) {
                    onOrderReceived(std::get<Order>(variant), shard_id);
                }
                else if (std::holds_alternative<TradeExecution>(variant)) {
                    onTradeReceived(std::get<TradeExecution>(variant), shard_id);
                }
                else {
                    logger_wrapper_->error(shard_id, "[RiskEngine] Unknown or malformed message received");
                }
            }
            else
                break;
        }
        if (!processed) {
            idle_strategy.idle();
        }
        processed = false;
    }
    logger_wrapper_->debug(shard_id, "[RiskEngine] runShard exiting");
}

void RiskEngine::onOrderReceived(const Order &order, int shard_id) {
    logger_wrapper_->debug(shard_id, "[RiskEngine] Received order");
    // Determine which shard this order belongs to
    int shard = static_cast<int>(order.account_id % NUM_SHARDS);

    // 1) Pre-trade checks
    bool passQty   = pretrade_checks_[shard].checkMaxOrderQty(order);
    bool passPos   = pretrade_checks_[shard].checkPositionLimit(order);
    // We don't have a reference price here; assume checkPriceBand not used for now

    if (!passQty) {
        logger_wrapper_->error(shard_id, "[RiskEngine] Order rejected: max qty exceeded for account {}", order.account_id);
        return;
    }
    if (!passPos) {
        logger_wrapper_->error(shard_id, "[RiskEngine] Order rejected: position limit for account {}", order.account_id);
        return;
    }

    // If passed, send the order to the matching engine (not implemented here).
    logger_wrapper_->debug(shard_id, "[RiskEngine] Order accepted: account {}, qty {}", order.account_id, order.quantity);
}

void RiskEngine::onTradeReceived(const TradeExecution &trade, int shard_id) {
    // Determine shard
    int shard = static_cast<int>(trade.account_id % NUM_SHARDS);

    // Post-trade update (positions, PnL, margin)
    posttrade_controls_[shard].onTrade(trade);

    // After updating positions, send a trade confirmation back if needed
    // (In this example, we simply log it)
    logger_wrapper_->debug(shard_id, "[RiskEngine] Trade received: account {}, qty {}, price {}", trade.account_id, trade.quantity, trade.price);
}