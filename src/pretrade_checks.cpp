//
// Created by muhammad-abdullah on 5/27/25.
//
// File: src/pretrade_checks.cpp
#include "pretrade_checks.h"

#include <iostream>

bool rms::PreTradeChecks::checkMaxOrderQty(const Order &order) {
    int shard = order.account_id % NUM_SHARDS;
    const auto &lim = instrument_limits_shards[shard][order.instrument_id];
    return order.quantity <= (int64_t)lim.max_order_qty;
}

bool rms::PreTradeChecks::checkPriceBand(const Order &order, double reference_price) {
    int shard = order.instrument_id % NUM_SHARDS;
    const auto &lim = instrument_limits_shards[shard][order.instrument_id];
    double tol = lim.price_tolerance_pct * reference_price;
    return std::abs(order.price - reference_price) <= tol;
}

bool rms::PreTradeChecks::checkPositionLimit(const Order &order) {
    int shard = order.account_id % NUM_SHARDS;
    auto &pos_map = position_store[shard];
    auto it = pos_map.find(order.instrument_id);
    int64_t curr_pos = (it == pos_map.end() ? 0LL : it->second.net_qty);
    std::cout << order.instrument_id << ", " << curr_pos << std::endl;
    const auto &lim = instrument_limits_shards[shard][order.instrument_id];
    return std::abs(curr_pos + order.quantity) <= (int64_t)lim.max_daily_position;
}