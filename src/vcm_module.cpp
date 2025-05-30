//
// Created by muhammad-abdullah on 5/27/25.
//
#include "vcm_module.h"
#include "data_types.h"
// File: src/vcm_module.cpp
#include "vcm_module.h"
#include <cmath>

void rms::VCMModule::onMarketData(uint32_t instrument_id, double best_bid, double best_ask) {
    // Simplified: do nothing
}

bool rms::VCMModule::checkSpread(const Order &order) {
    int inst_id = order.instrument_id;
    const auto &lim = instrument_limits_shards[inst_id % NUM_SHARDS][inst_id];
    double spread = 0.0; // stub
    return spread <= lim.max_spread_ticks;
}

bool rms::VCMModule::checkVolatility(const Order &order) {
    return true; // stub always pass
}