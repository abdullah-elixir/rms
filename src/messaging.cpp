// File: src/messaging.cpp
#include "messaging.h"
#include "utils/logger.h"
#include <iostream>
#include <cstring>
#include <FragmentAssembler.h>
#include <chrono>

using namespace rms;

static constexpr const char *CHANNEL_IN  = "aeron:udp?endpoint=localhost:40123"; // where orders arrive
static constexpr const char *CHANNEL_OUT = "aeron:udp?endpoint=localhost:40124"; // where trade confirmations go
static constexpr const char *CHANNEL_IPC  = "aeron:ipc"; // where orders arrive
static constexpr std::int32_t STREAM_ID = 1001;
static constexpr std::chrono::duration<long, std::milli> SLEEP_IDLE_MS(1);

Messaging::Messaging() = default;

Messaging::~Messaging() {
    shutdown();
}

bool Messaging::initialize(OrderCallback ocb, TradeCallback tcb) {
    orderCb_ = std::move(ocb);
    tradeCb_ = std::move(tcb);

    try {
        aeron::Context context;
        aeron_ = aeron::Aeron::connect(context);

        // Create a subscription for incoming messages (orders/trades)
        std::int64_t id = aeron_->addSubscription(CHANNEL_IPC, STREAM_ID);

        subscription_ = aeron_->findSubscription(id);
        // wait for the subscription to be valid
        while (!subscription_)
        {
            std::this_thread::yield();
            subscription_ = aeron_->findSubscription(id);
        }
        utils::logInfo("[Messaging] Subscribed. Sub Id: " + std::to_string(id));
        // // Create a publication for outgoing messages (trade confirmations)
        // id = aeron_->addPublication(CHANNEL_OUT, STREAM_ID);
        //
        // publication_ = aeron_->findPublication(id);
        // // wait for the publication to be valid
        // while (!publication_)
        // {
        //     std::this_thread::yield();
        //     publication_ = aeron_->findPublication(id);
        // }

    }
    catch (const std::exception &ex) {
        std::cerr << "[Messaging] Aeron initialization failed: " << ex.what() << std::endl;
        return false;
    }

    running_ = true;
    listenerThread_ = std::thread(&Messaging::listenerLoop, this);
    utils::logInfo("[Messaging] Aeron initialized and listener thread started");
    return true;
}

aeron::fragment_handler_t Messaging::fragHandler() {
    return  [&](const aeron::AtomicBuffer &buffer,
                           std::int32_t offset,
                           std::int32_t length,
                           const aeron::Header &header) {
        if (length < sizeof(std::uint8_t)) {
            return; // too small to read any header
        }
        std::cout<<"received aeron::Fragment buffer: "<<buffer.buffer()<<std::endl;
        // First byte indicates message type: 1 = Order, 2 = TradeExecution
        std::uint8_t msgType = buffer.getUInt8(offset);
        std::cout << msgType<< ","<<length <<","<< 1 + sizeof(Order)<< std::endl;
        if (true) {
            // Deserialize Order
            Order order;
            std::memcpy(&order, buffer.buffer() + offset + 1, sizeof(Order));
            if (orderCb_) {
                orderCb_(order);
            }
        }
        else if (msgType == 2 && length >= (1 + sizeof(TradeExecution))) {
            // Deserialize TradeExecution
            TradeExecution trade;
            std::memcpy(&trade, buffer.buffer() + offset + 1, sizeof(TradeExecution));
            if (tradeCb_) {
                tradeCb_(trade);
            }
        }
        else {
            utils::logError("[Messaging] Unknown or malformed message received");
        }
    };
}


void Messaging::listenerLoop() {
    utils::logInfo("[Messaging] listenerLoop started");
    // Buffer for deserializing incoming messages
    static thread_local std::array<std::uint8_t, 256> msgBuffer;
    aeron::FragmentAssembler fragmentAssembler(fragHandler());
    aeron::fragment_handler_t handler = fragmentAssembler.handler();
    aeron::SleepingIdleStrategy sleepStrategy(SLEEP_IDLE_MS);
    while (running_) {
        // Poll up to 10 fragments per iteration
        std::int32_t fragmentsRead = subscription_->poll(handler, 10);
        sleepStrategy.idle(fragmentsRead);
    }
    utils::logInfo("[Messaging] Listener thread exiting");
}

bool Messaging::sendTradeExecution(const TradeExecution &trade) {
    // Serialize: 1 byte for type, then struct bytes
    std::uint8_t msgType = 2;
    std::uint8_t bufferData[1 + sizeof(TradeExecution)];
    bufferData[0] = msgType;
    std::memcpy(bufferData + 1, &trade, sizeof(TradeExecution));

    aeron::AtomicBuffer srcBuffer(bufferData, sizeof(bufferData));
    std::int64_t result = publication_->offer(srcBuffer, 0, sizeof(bufferData));
    if (result < 0) {
        utils::logError("[Messaging] Failed to send trade execution; offer returned " + std::to_string(result));
        return false;
    }
    return true;
}

void Messaging::shutdown() {
    if (!running_) return;
    running_ = false;
    if (listenerThread_.joinable()) {
        listenerThread_.join();
    }
    publication_.reset();
    subscription_.reset();
    aeron_.reset();
    utils::logInfo("[Messaging] Shutdown complete");
}
