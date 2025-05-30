//
// Created by muhammad-abdullah on 5/27/25.
//

// File: include/rms/pretrade_checks.hpp
#pragma once
#include "data_types.h"

namespace rms {
    struct Order {
        uint32_t account_id;
        uint32_t instrument_id;
        int64_t quantity;
        double price;
    };

    class PreTradeChecks {
    public:
        bool checkMaxOrderQty(const Order &order);
        bool checkPriceBand(const Order &order, double reference_price);
        bool checkPositionLimit(const Order &order);
    };
}