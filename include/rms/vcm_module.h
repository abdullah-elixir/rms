//
// Created by muhammad-abdullah on 5/27/25.
//

// File: include/rms/vcm_module.hpp
#pragma once
#include "data_types.h"
#include "pretrade_checks.h"

namespace rms {
    class VCMModule {
    public:
        void onMarketData(uint32_t instrument_id, double best_bid, double best_ask);
        bool checkSpread(const Order &order);
        bool checkVolatility(const Order &order);
    };
}