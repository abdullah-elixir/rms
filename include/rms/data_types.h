//
// Created by muhammad-abdullah on 5/27/25.
//
// File: include/rms/data_types.hpp
#pragma once
#include <cstdint>
#include <array>
#include <folly/container/F14Map.h>

constexpr int NUM_SHARDS = 4;
constexpr int NUM_INSTRUMENTS = 1024;
constexpr int ACCOUNTS_PER_SHARD = 256;

struct InstrumentLimits {
    uint32_t  max_order_qty = 100;
    double    max_order_notional = 1000000.0;
    double    price_tolerance_pct = 0.02;
    double    max_spread_ticks = 5;
    double    init_margin_pct = 0.05;
    double    maint_margin_pct = 0.025;
    uint32_t  max_daily_position = 1000;
} __attribute__((aligned(64)));

struct AccountLimits {
    uint32_t max_order_rate_per_sec = 100;
    uint32_t max_concurrent_orders = 100;
    double   max_leverage = 10.0;
    double   max_drawdown_pct = 0.1;
    bool     kill_switch = false;
} __attribute__((aligned(64)));

struct Position {
    int64_t net_qty = 0;
    double  avg_entry_price = 0.0;
    double  realized_pnl = 0.0;
    double  unrealized_pnl = 0.0;
    double  peak_equity = 0.0;
} __attribute__((aligned(64)));

struct Order {
    uint64_t order_id;
    uint32_t account_id;
    uint32_t instrument_id;
    int64_t  quantity;
    double   price;
    char     symbol[16];
    char     side[4];   // "BUY" or "SELL"
};

struct TradeExecution {
    uint64_t order_id;
    uint32_t account_id;
    uint32_t instrument_id;
    int64_t  quantity;
    double   price;
    char     symbol[16];
    bool     is_buy;
    uint64_t trade_id;
}__attribute__((aligned(64)));

using InstrumentLimitsShard = std::array<InstrumentLimits, NUM_INSTRUMENTS>;
using AccountLimitsShard = std::array<AccountLimits, ACCOUNTS_PER_SHARD>;

extern InstrumentLimitsShard instrument_limits_shards[NUM_SHARDS];
extern AccountLimitsShard account_limits_shards[NUM_SHARDS];
extern std::array<folly::F14FastMap<uint32_t, Position>, NUM_SHARDS> position_store;