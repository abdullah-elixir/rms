//
// Created by muhammad-abdullah on 5/27/25.
//

// File: include/rms/messaging.hpp
#pragma once
#include <string>
#include <functional>
#include "posttrade_controls.h"

namespace rms {
    using OrderCallback = std::function<void(const Order &)>;
    using TradeCallback = std::function<void(const TradeExecution &)>;

    class Messaging {
    public:
        bool initialize(OrderCallback ocb, TradeCallback tcb);
        void run();
        void shutdown();
    };
}
