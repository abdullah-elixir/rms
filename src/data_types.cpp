//
// Created by muhammad-abdullah on 5/27/25.
//

// File: src/data_types.cpp
#include "data_types.h"

InstrumentLimitsShard instrument_limits_shards[NUM_SHARDS];
AccountLimitsShard account_limits_shards[NUM_SHARDS];
std::array<folly::F14FastMap<uint32_t, Position>, NUM_SHARDS> position_store;