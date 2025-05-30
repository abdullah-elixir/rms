//
// Created by muhammad-abdullah on 5/27/25.
//

// File: include/rms/posttrade_controls.hpp
#pragma once
#include "data_types.h"
#include "pretrade_checks.h"

namespace rms {
    struct TradeExecution {
        uint32_t account_id;
        uint32_t instrument_id;
        int64_t quantity;
        double price;
        bool is_buy;
    };

    class PostTradeControls {
    public:
        void onTrade(const TradeExecution &trade);
    };
}