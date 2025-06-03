//
// Created by muhammad-abdullah on 5/27/25.
//

// File: include/rms/posttrade_controls.hpp
#pragma once
#include "data_types.h"
#include "pretrade_checks.h"

namespace rms {
    class PostTradeControls {
    public:
        void onTrade(const TradeExecution &trade);
    };
}