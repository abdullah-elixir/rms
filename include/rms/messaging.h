// File: include/rms/messaging.hpp
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include<array>
#include <thread>
#include <Aeron.h>
#include <Context.h>
#include <Subscription.h>
#include <Publication.h>
#include <concurrent/AtomicBuffer.h>
#include "data_types.h"      // For Order, TradeExecution, etc.
#include "sharded_queue.h"
#include "logger.h"
#include "loggerwrapper.h"

namespace rms {

    using OrderCallback = std::function<void(const Order &)>;
    using TradeCallback = std::function<void(const TradeExecution &)>;

    class Messaging {
    public:
        Messaging();
        ~Messaging();

        /// Initialize Aeron, set up Publication & Subscription, and register callbacks.
        /// Returns false if Aeron setup fails.
        bool initialize(std::unique_ptr<LoggerWrapper>&);

        /// Shutdown Aeron and stop listener thread.
        void shutdown();

        /// Send a TradeExecution message out via Aeron Publication.
        bool sendTradeExecution(const TradeExecution& trade);

        ///fragment handler
        aeron::fragment_handler_t fragHandler();

        ///getqueue
        std::array<ShardedQueue, NUM_SHARDS>& getQueue();

    private:
        /// Listener loop that polls Aeron Subscription.
        void listenerLoop();

        std::atomic_bool running_{false};
        std::thread listenerThread_;


        OrderCallback orderCb_;
        TradeCallback tradeCb_;

        std::shared_ptr<aeron::Aeron> aeron_;
        std::shared_ptr<aeron::Subscription> subscription_;
        std::shared_ptr<aeron::Publication> publication_;

        std::array<ShardedQueue, NUM_SHARDS> sharded_queue;

        //log wrapper
        LoggerWrapper* logWrapper;
    };

}  // namespace rms
